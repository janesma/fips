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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
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

	name = talloc_size (ctx, name_len - 1);
	if (name == NULL) {
		fprintf (stderr, "Out of memory.\n");
		exit (1);
	}

	name_len = readlink (link, name, name_len);
	if (name_len < 0) {
		fprintf (stderr, "Failed to readlink %s: %s\n", link,
			 strerror (errno));
		exit (1);
	}

	name[name_len + 1] = '\0';

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

	fd = open (program, O_RDONLY, 0);
	if (fd < 0) {
		fprintf (stderr, "Failed to open %s: %s\n", program,
			 strerror (errno));
		exit (1);
	}

	if (elf_version (EV_CURRENT ) == EV_NONE) {
		fprintf (stderr, "Failed to initialize elf library: %s\n",
			 elf_errmsg (-1));
		exit (1);
	}

	elf = elf_begin (fd, ELF_C_READ, NULL);
	if (elf == NULL) {
		fprintf (stderr, "Call to elf_begin on %s failed: %s\n",
			 program, elf_errmsg(-1));
		exit (1);
	}

	if (elf_kind (elf) != ELF_K_ELF) {
		fprintf (stderr, "Not an ELF object: %s\n", program);
		exit (1);
	}

	if (gelf_getehdr (elf, &ehdr) == NULL) {
		fprintf (stderr, "getehdr on %s failed: %s\n",
			 program, elf_errmsg (-1));
		exit (1);
	}

	class = gelf_getclass (elf);

	if (class == ELFCLASSNONE) {
		fprintf (stderr, "getclass on %s failed: %s\n", program,
			 elf_errmsg (-1));
		exit (1);
	}

	if (class == ELFCLASS32)
		return 32;
	else
		return 64;

}

/* Find the appropriate path to the libfips wrapper.
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
 * Returns: a string talloc'ed to 'ctx'
 */
static char *
find_libfips_path (void *ctx, const char *program)
{
	char *bin_path, *library, *lib_path;
	int bits;

	bits = elf_bits (program);

	library = talloc_asprintf(ctx, "libfips-%d.so", bits);

	bin_path = get_bin_name (ctx);

	chop_trailing_path_component (bin_path);

	lib_path = talloc_asprintf(ctx, "%s/%s", bin_path, library);

	if (exists (lib_path))
		return lib_path;

	talloc_free (lib_path);

	lib_path = talloc_asprintf(ctx, "%s/" BINDIR_TO_LIBFIPSDIR "/%s",
				   bin_path, library);

	if (exists (lib_path))
		return lib_path;

	fprintf (stderr, "Error: Failed to find library %s.\n", library);
	fprintf (stderr, "Looked in both:\n"
		 "\t%s\n"
		 "and\n"
		 "\t%s/" BINDIR_TO_LIBFIPSDIR "\n", bin_path, bin_path);
	exit (1);
}

/* After forking, set LD_PRELOAD to preload libfips-{32,64}.so within
 * child environment, then exec given arguments.
 */
static int
fork_exec_with_fips_preload_and_wait (char * const argv[])
{
	pid_t pid;
	int i, status;

	pid = fork ();

	/* Child */
	if (pid == 0) {
		void *ctx = talloc_new (NULL);
		char *lib_path;

		lib_path = find_libfips_path (ctx, argv[0]);

		setenv ("LD_PRELOAD", lib_path, 1);

		talloc_free (ctx);
		
		execvp (argv[0], argv);
		fprintf (stderr, "Failed to execute:");
		for (i = 0; argv[i]; i++) {
			fprintf (stderr, " %s", argv[i]);
		}
		fprintf (stderr, "\n");
		exit (1);
	}

	/* Parent */
	waitpid (pid, &status, 0);
	if (WIFEXITED (status)) {
		return (WEXITSTATUS (status));
	}
	if (WIFSIGNALED (status)) {
		fprintf (stderr, "Child terminated by signal %d\n",
			 WTERMSIG (status));
	}
	return 1;
}

int
execute_with_fips_preload (int argc, char * const argv[])
{
	char **execvp_args;
	int i;

	execvp_args = malloc((argc + 1) * sizeof(char *));
	if (execvp_args == NULL) {
		fprintf (stderr, "Out of memory,\n");
		return 1;
	}

	for (i = 0; i < argc; i++) {
		execvp_args[i] = argv[i];
	}

	/* execvp needs final NULL */
	execvp_args[i] = NULL;

	return fork_exec_with_fips_preload_and_wait (execvp_args);
}
