#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>

#include "list.h"

#define TEMP_STR_LEN 32

int display_attr(char *buf, size_t buf_size, const char *file) {
	struct stat st;
	int ret;
	char *m;

	errno = 0;
	ret = lstat(file, &st);
	if (ret == -1) {
		fprintf(stderr, "%s ", file);
		perror("lstat error");
		return -1;
	}
    /**
     * file mode
     * st_mode sample(in oct):
     * 0   12   0  7     4     4
     *     l       rwx   rw-    rw-
     * oct type ?? owner group others
     * see man page for details($man lstat)
     */
	if (S_ISREG(st.st_mode))
		m = "-";
	else if (S_ISDIR(st.st_mode))
		m = "d";
	else if (S_ISCHR(st.st_mode))
		m = "c";
	else if (S_ISBLK(st.st_mode))
		m = "b";
	else if (S_ISFIFO(st.st_mode))
		m = "f";
	else if (S_ISLNK(st.st_mode))
		m = "l";
	else if (S_ISSOCK(st.st_mode))
		m = "s";
	else    /* just kidding */
		m = "-";
	strcat(buf, m);


    /*file permission*/
    int n;
    int temp;
    int tmp;
    for (n=8; n>=0; n--)
    {
        /**
         * temp:
         * dec 256  128  64   32   16   8    4    2    1
         * oct 0400 0200 0100 0040 0020 0010 0004 0002 0001
         */
        temp = 1<<n;
        tmp = st.st_mode & temp;
        if (tmp)
        // if (st.st_mode & (1<<n))
        {
            switch (n%3)
            {
				case 2: strcat(buf, "r"); break;
				case 1: strcat(buf, "w"); break;
				case 0: strcat(buf, "x"); break;
            }
        }
        else
            strcat(buf, "-");
    }

	char temp_str[TEMP_STR_LEN];

    /* number of hard link */
	bzero(temp_str, TEMP_STR_LEN);
	sprintf(temp_str, "%3ld ", st.st_nlink);
	strcat(buf, temp_str);

    /* user name */
	bzero(temp_str, TEMP_STR_LEN);
    struct passwd *pw = getpwuid(st.st_uid);
	sprintf(temp_str, "%8s ", pw->pw_name);
	strcat(buf, temp_str);

    /* group name */
	bzero(temp_str, TEMP_STR_LEN);
    struct group *grp = getgrgid(st.st_gid);
	sprintf(temp_str, "%8s ", grp->gr_name);
	strcat(buf, temp_str);

    /* file size */
	bzero(temp_str, TEMP_STR_LEN);
	sprintf(temp_str, "%11ld ", st.st_size);
	strcat(buf, temp_str);

    /* last modify time */
	bzero(temp_str, TEMP_STR_LEN);
    struct tm *t;
    t = localtime(&(st.st_mtime));
	sprintf(temp_str, "%4d.%02d.%02d %02d:%02d ", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min);
	strcat(buf, temp_str);

    /* Collect all */
	strcat(buf, file);
	strcat(buf, "\r\n");
	return 0;
}

int list_cwd(char *buf, size_t buf_size) {
	DIR *dir = NULL;
	char cwd[PATH_MAX];
	struct dirent *de = NULL;

	bzero(buf, buf_size);

	dir = opendir(getcwd(cwd, PATH_MAX));
	while ((de = readdir(dir)) != NULL) {
		if (buf_size - strlen(buf) < 63 + NAME_MAX)
			return -1;
		display_attr(buf, buf_size, de->d_name);
	}
	closedir(dir);
/*	char *p;
	for (p = buf; *p != '\0'; p++)
		printf("%c", *p);*/
	return 0;
}
