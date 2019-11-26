#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE  600

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "c11-threads.h"

static ssize_t safe_read (int fd, void *buf, size_t count)
{
	ssize_t n;

	while ((n = read (fd, buf, count)) < 0 && errno == EINTR) {}

	return n;
}

static ssize_t safe_write (int fd, const void *buf, size_t count)
{
	ssize_t avail, n;
	const char *p;

	for (p = buf, avail = count; avail > 0; p += n, avail -= n) {
		while ((n = write (fd, p, count)) < 0 && errno == EINTR) {}

		if (n < 0)
			return n;
	}

	return count;
}

#define BUFSIZE  512

static void no_filter (int in, int out)
{
	char buf[BUFSIZE];
	ssize_t n;

	while ((n = safe_read (in, buf, sizeof (buf))) > 0 &&
	       safe_write (out, buf, n) == n) {}
}

static void csi_filter (int in, int out)
{
	enum state { INIT, ESCAPE, CSI } state = INIT;
	/* reserve one extra byte for delayed ESC symbol in output buffer */
	char ibuf[BUFSIZE - 1], obuf[BUFSIZE], *end, *p, *q;
	ssize_t n;

	while ((n = safe_read (in, ibuf, sizeof (ibuf))) > 0) {
		for (end = ibuf + n, p = ibuf, q = obuf; p < end; ++p)
			switch (state) {
			case INIT:
				if (*p == 033) {
					state = ESCAPE;
					break;
				}

				*q++ = *p;
				break;

			case ESCAPE:
				if (*p == 0133) {
					state = CSI;
					break;
				}

				*q++ = 033;  /* write delayed ESC symbol */
				*q++ = *p;
				state = INIT;
				break;

			case CSI:
				if (*p >= 0100 && *p <= 0176)
					state = INIT;

				break;
			}

		if (safe_write (out, obuf, n = (q - obuf)) != n)
			break;
	}
}

static int run (char *argv[], pid_t *child)
{
	int master, slave;
	const char *device;

	if ((master = posix_openpt (O_RDWR | O_NOCTTY)) < 0)
		return -1;

	if (grantpt (master) != 0 || unlockpt (master) != 0 ||
	    (device = ptsname (master)) == NULL ||
	    (slave = open (device, O_RDWR)) < 0)
		goto no_slave;

	if ((*child = fork ()) < 0)
		goto no_fork;

	if (*child > 0) {
		close (slave);
		return master;
	}

	close (master);

	dup2 (slave, 0);
	dup2 (slave, 1);
	dup2 (slave, 2);

	if (slave > 2)
		close (slave);

	setsid ();
	ioctl (0, TIOCSCTTY, 1);

	execvp (argv[0], argv);
	perror ("cannot run program");
	exit (1);
no_fork:
	close (slave);
no_slave:
	close (master);
	return -1;
}

static int no_filter_proc (void *data)
{
	int *file = data;

	no_filter (file[0], file[1]);
	return 0;
}

static int csi_filter_proc (void *data)
{
	int *file = data;

	csi_filter (file[0], file[1]);
	return 0;
}

int main (int argc, char *argv[])
{
	pid_t child;
	int master, status = 1;

	struct termios to, tn;

	int f1[2], f2[2];
	thrd_t t1, t2;

	if (argc < 2) {
		fprintf (stderr, "usage:\n\tterm-filter program [args...]\n");
		return 1;
	}

	if ((master = run (argv + 1, &child)) < 0) {
		perror ("cannot run program");
		return 1;
	}

	if (isatty (0)) {
		tcgetattr (0, &to);
		tn = to;
		cfmakeraw (&tn);
		tcsetattr (0, TCSANOW, &tn);
	}

	f1[0] = 0;
	f1[1] = master;
	f2[0] = master;
	f2[1] = 1;

	thrd_create (&t1, no_filter_proc,  f1);
	thrd_create (&t2, csi_filter_proc, f2);

	thrd_detach (t1);
	thrd_detach (t2);

	sleep (1);  /* wait a bit for error messages from child if any */

	if (waitpid (child, &status, 0) != child)
		perror ("cannot get program status");
	else
		status = WIFEXITED (status) ? WEXITSTATUS (status) : 1;

	if (isatty (0))
		tcsetattr (0, TCSANOW, &to);

	return status;
}
