#include "vfs.h"

int walkTheDir (char * newrootdir);


int walkTheDir (char * newrootdir)
{
    DIR * dirp         = NULL;
    struct dirent * dp = NULL;
    struct stat sb;
    char * buffer      = NULL;

#ifdef DEBUG
    scar_log (1, "...entered directory: %s\n", newrootdir);
#endif


    dirp = opendir(newrootdir);

    while ((dp = readdir(dirp)) != NULL)
    {
        buffer = malloc (sizeof (char) * (PATH_MAX + 1));

        /* No current dir or parent dir */
        if ( (strcmp(dp -> d_name, ".") == 0) ||
             (strcmp(dp -> d_name, "..") == 0) )
            continue;

        /* Stat the current file */
        snprintf (buffer, PATH_MAX, "%s/%s", newrootdir, dp -> d_name);
        lstat(buffer, &sb);

        /* Special files */
        if ((S_ISCHR(sb.st_mode)) || (S_ISBLK(sb.st_mode)) || (S_ISFIFO(sb.st_mode)) || (S_ISSOCK (sb.st_mode)))
        {
            scar_log (1, "figure out that \"%s\" is a special file or thing\n", dp -> d_name);
            continue;
        }

        /* Symlink */
        else if (S_ISLNK(sb.st_mode))
        {
            scar_log (1, "figure out that \"%s\" is a symlink\n", dp -> d_name);
            continue;
        }

        /* Directories */
        else if (S_ISDIR(sb.st_mode))
        {
            scar_log (1, "figure out that \"%s\" is a directory\n", dp -> d_name);
            walkTheDir (buffer);
        }

        /* Regular file */
        else if (S_ISREG(sb.st_mode))
        {
            scar_log (1, "figure out that \"%s\" is a regular file\n", dp -> d_name);
        }
    }

    (void)closedir(dirp);

    free(dp);

    return 0;
}


void * vfs_main (void * arg)
{
    char * rootpath = (char *) arg;

    walkTheDir (rootpath);
    return 0;
}

