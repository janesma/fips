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

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static int
fork_exec_and_wait (char * const argv[])
{
	pid_t pid;
	int i, status;

	pid = fork ();

	/* Child */
	if (pid == 0) {
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
execute (int argc, char * const argv[])
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

	return fork_exec_and_wait (execvp_args);
}
