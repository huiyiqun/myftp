#include <linux/limits.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "close_fd.h"

void close_open_fds()
{
    long fd, maxfd;
    char fdpath[PATH_MAX], *endp;
    struct dirent *dent;
    DIR *dirp;
    int len;

    /* Check for a /proc/$$/fd directory. */
    len = snprintf(fdpath, sizeof(fdpath), "/proc/%ld/fd", (long)getpid());
    if (len > 0 && (size_t)len <= sizeof(fdpath) && (dirp = opendir(fdpath))) {
	while ((dent = readdir(dirp)) != NULL) {
	    fd = strtol(dent->d_name, &endp, 10);
	    if (dent->d_name != endp && *endp == '\0' && fd != dirfd(dirp)) {
		(void) close((int) fd);
            }
	}
	(void) closedir(dirp);
    } else {
	/*
	 * Fall back on sysconf().  We avoid checking
	 * resource limits since it is possible to open a file descriptor
	 * and then drop the rlimit such that it is below the open fd.
	 */
	maxfd = sysconf(_SC_OPEN_MAX);

	for (fd = 0; fd < maxfd; fd++)
	    (void) close((int) fd);
    }
}
