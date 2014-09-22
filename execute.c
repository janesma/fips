/* Copyright © 2013, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "fips.h"

#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <talloc.h>

#include <linux/limits.h>

#include <fcntl.h>
#include <gelf.h>

#include "execute.h"
#include "xmalloc.h"

/* Terminate a string representing a filename at the final '/' to
 * eliminate the final filename component, (leaving only the directory
 * portions of the original path).
 *
 * Notes: A path containing no '/' character will not be modified.
 *        A path consisting only of "/" will not be modified.
 */
static void
chop_trailing_path_component (char *path)
{
	char *slash;

	slash = strrchr (path, '/');

	if (slash == NULL)
		return;

	if (slash == path)
		return;

	*slash = '\0';
}

/* Find the absolute path of the currently executing binary.
 *
 * Returns: a string talloc'ed to 'ctx'
 */
static char *
get_bin_name (void *ctx)
{
	const char *link = "/proc/self/exe";
	char *name;

	/* Yes, PATH_MAX is cheesy. I would have preferred to have
	 * used lstat and read the resulting st_size, but everytime I
	 * did that with /proc/self/exe I got a value of 0, (whereas
	 * with a "real" symbolic link I make myself I get the length
	 * of the filename being linked to). Go figure. */
	int name_len = PATH_MAX + 1;

	name = talloc_size (ctx, name_len);
	if (name == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	name_len = readlink (link, name, name_len - 1);
	if (name_len < 0) {
		fprintf (stderr, "fips: Error: Failed to readlink %s: %s\n", link,
			 strerror (errno));
		exit (1);
	}

	name[name_len] = '\0';

	return name;
}

/* Does path exist? */
static int
exists (const char *path)
{
	struct stat st;
	int err;

	err = stat (path, &st);

	/* Failed to stat. It either doesn't exist, or might as well not. */
	if (err == -1)
		return 0;

	return 1;
}

/* Given a program name, search the PATH environment variable and
 * return the first absolute path to 'program'.
 *
 * Returns: A string talloc'ed to 'ctx'.
 *
 * Note: This function aborts the current program if 'program' cannot
 * be located by searching PATH.
 */
static char *
search_path_for_program (void *ctx, const char *program)
{
	char *orig_path, *path, *colon, *dir, *candidate;
	void *local = talloc_new (ctx);

	/* If the program name already contains a slash, then this is
	 * an absolute (or relative) path. Either way, we don't search
	 * PATH, since we can directly open this filename. */
	if (strchr (program, '/'))
	    return talloc_strdup (ctx, program);

	orig_path = path = getenv ("PATH");

	while (*path) {
		colon = strchr (path, ':');

		if (colon) {
			dir = talloc_strndup (local, path, colon - path);
			path = colon + 1;
		} else {
			dir = path;
			path = path + strlen (path);
		}

		candidate = talloc_asprintf(local, "%s/%s", dir, program);

		if (exists (candidate)) {
			talloc_steal (ctx, candidate);
			talloc_free (local);
			return candidate;
		} else {
			talloc_free (candidate);
		}
	}

	fprintf (stderr, "Cannot find program %s (looked in %s)\n",
		 program, orig_path);
	exit (1);
}

/* Is the given elf program 32 or 64 bit?
 *
 * Note: This function aborts the current program if 'program' cannot
 * be opened as a valid ELF file. */
static int
elf_bits (const char *program)
{
	Elf *elf;
	GElf_Ehdr ehdr;
	int fd, class;
	void *local = talloc_new (NULL);
	char *absolute_program = search_path_for_program (local, program);

	fd = open (absolute_program, O_RDONLY, 0);
	if (fd < 0) {
		fprintf (stderr, "fips: Failed to open %s: %s\n", absolute_program,
			 strerror (errno));
		exit (1);
	}

	if (elf_version (EV_CURRENT ) == EV_NONE) {
		fprintf (stderr, "fips: Failed to initialize elf library: %s\n",
			 elf_errmsg (-1));
		exit (1);
	}

	elf = elf_begin (fd, ELF_C_READ, NULL);
	if (elf == NULL) {
		fprintf (stderr, "fips: Call to elf_begin on %s failed: %s\n",
			 absolute_program, elf_errmsg(-1));
		exit (1);
	}

	if (elf_kind (elf) != ELF_K_ELF) {
		fprintf (stderr, "fips: Not an ELF object: %s\n", absolute_program);
		exit (1);
	}

	if (gelf_getehdr (elf, &ehdr) == NULL) {
		fprintf (stderr, "fips: getehdr on %s failed: %s\n",
			 absolute_program, elf_errmsg (-1));
		exit (1);
	}

	class = gelf_getclass (elf);

	if (class == ELFCLASSNONE) {
		fprintf (stderr, "fips: getclass on %s failed: %s\n",
			 absolute_program, elf_errmsg (-1));
		exit (1);
	}

	talloc_free (local);

	if (class == ELFCLASS32)
		return 32;
	else
		return 64;

}

/* Execute "program" in a pipe, reads its first line of output on
 * stdout, and returns that as a string (discarding any further
 * output).
 *
 * Returns NULL if the program failed to execute for any reason.
 *
 * NOTE: The caller should free() the returned string when done with
 * it.
 */
static char *
read_process_output_one_line (const char *program)
{
	FILE *process;
	int status;
	char *line = NULL;
	size_t len = 0;
	ssize_t bytes_read;

	process = popen (program, "r");
	if (process == NULL)
		return NULL;

	bytes_read = getline (&line, &len, process);

	status = pclose (process);
	if (! WIFEXITED (status))
		return NULL;

	if (WEXITSTATUS (status))
		return NULL;

	if (bytes_read == -1)
		return NULL;

	if (bytes_read) {
		if (line[strlen(line)-1] == '\n')
			line[strlen(line)-1] = '\0';
		return line;
	} else {
		return NULL;
	}
}


/* Find the appropriate path to fips libraries, and set corresponding
 * environment variables.
 *
 * This involves, first, examining the elf header of the 'program'
 * binary to be executed to know whether we should look for
 * libfips-32.so or libfips-64.so.
 *
 * Next, we find the absolute patch containing the library as follows:
 *
 *   1. Look in same directory as current executable image
 *
 *      This is to support running from the source directory, without
 *      having installed anything.
 *
 *   2. Look in relative path from $(foo)/$(bindir) to
 *      $(foo)/$(libdir)/fips based on $(foo) from current executable
 *      image and configured $(bindir) and $(libdir).
 *
 *      We do this rather than looking directly at the configured
 *      $(libdir) to support cases where the application may have been
 *      moved after being installed, (in particular, we want to be
 *      particularly careful not to mix one program with a different
 *      wrapper---so this "nearest search" should most often be
 *      correct.
 *
 * sets LD_LIBRARY_PATH to point at directory containing fips libGL
 * sets FIPS_GL to point at real libGL
 * sets FIPS_EGL to point at real libEGL
 *
 */
static 
void set_lib_environment(const char *program)
{
	void *ctx = talloc_new (NULL);
	char *bin_path, *library, *lib_path, *find_lib_binary, *find_lib_invoke, *ld_path;
    const int bits = elf_bits(program);
    const char * lib_dir = (bits == 64) ? LIB64_DIR : LIB32_DIR;

	library = talloc_asprintf(ctx, "libfips-%d.so", bits);

	bin_path = get_bin_name (ctx);

	chop_trailing_path_component (bin_path);

	lib_path = talloc_asprintf(ctx, "%s/%s", bin_path, library);

	if (! exists (lib_path))
    {
        lib_path = talloc_asprintf(ctx, "%s/../lib/fips/%s", bin_path, library);
        if (! exists (lib_path))
        {
            fprintf (stderr, "fips: Error: Failed to locate libfips-%d.so\n", bits);
            exit(-1);
        }
    }
    setenv ("FIPS_LIBFIPS", lib_path, 1);


    find_lib_binary = talloc_asprintf(ctx, "%s/fips-find-lib-%d", bin_path, bits);
    if (! exists (find_lib_binary))
    {
        fprintf (stderr, "fips: Error: Failed to locate fips-find-lib-%d.so\n", bits);
        exit(-1);
    }

    find_lib_invoke = talloc_asprintf(ctx, "%s EGL",
                                      find_lib_binary);
    lib_path = read_process_output_one_line (find_lib_invoke);
    if (lib_path == NULL)
    {
        fprintf (stderr, "fips: Error: determine target EGL.  Execute `%s EGL` "
                 "to reproduce error.\n", find_lib_binary);
        exit(-1);
    }

    setenv ("FIPS_EGL", lib_path, 1);
    free(lib_path);

    lib_path = talloc_asprintf(ctx, "%s/%s",
                               bin_path, lib_dir);

    find_lib_invoke = talloc_asprintf(ctx, "%s GL",
                                      find_lib_binary);
    lib_path = read_process_output_one_line (find_lib_invoke);
    if (lib_path == NULL)
    {
        fprintf (stderr, "fips: Error: determine target GL.  Execute `%s GL` "
                 "to reproduce error.\n", find_lib_binary);
        exit(-1);
    }

    setenv ("FIPS_GL", lib_path, 1);
    free(lib_path);

    lib_path = talloc_asprintf(ctx, "%s/%s",
                               bin_path, lib_dir);

    if (! exists (lib_path))
    {
        lib_path = talloc_asprintf(ctx, "%s/../lib/fips/%s",
                                   bin_path, lib_dir);
        if (! exists (lib_path))
        {
            fprintf (stderr, "fips: Error: Failed to locate directory containing "
                     "fips libGL.so.\n");
            exit(-1);
        }
    }
	ld_path = getenv ("LD_LIBRARY_PATH");
    if (!ld_path)
        setenv ("LD_LIBRARY_PATH", lib_path, 1);
    else
    {
        const char * appended_path = talloc_asprintf(ctx, "%s:%s", lib_path, ld_path);
        setenv ("LD_LIBRARY_PATH", appended_path, 1);
    }
    talloc_free(ctx);
}

int
execute_with_fips_wrapper (int argc, char * const argv[])
{
	void *ctx = talloc_new (NULL);
	char **execvp_args;
	int i;

	execvp_args = xmalloc((argc + 1) * sizeof(char *));

	for (i = 0; i < argc; i++) {
		execvp_args[i] = argv[i];
	}

	/* execvp needs final NULL */
	execvp_args[i] = NULL;

    set_lib_environment(argv[0]);

	talloc_free (ctx);
		
	execvp (argv[0], argv);
	fprintf (stderr, "Failed to execute:");
	for (i = 0; argv[i]; i++) {
		fprintf (stderr, " %s", argv[i]);
	}
	fprintf (stderr, "\n");
	exit (1);
}
