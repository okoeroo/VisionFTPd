#include <pthread.h>
#include "vfs.h"


vfs_t * VFS_unmarshall (unsigned char * blob)
{
}

unsigned char * VFS_marshall (vfs_t * vfs)
{
}


int walkTheDir (char * newrootdir, char * chrootdir, vfs_t ** vfs_parent)
{
    DIR * dirp         = NULL;
    struct dirent * dp = NULL;
    struct stat sb;
    char * buffer      = NULL;
    int    res         = 0;

    vfs_t *  vfs_node  = NULL;
    turl_t * turl      = NULL;


    /* Open new directory entry */
    dirp = opendir(newrootdir);

    while ((dp = readdir(dirp)) != NULL)
    {
        /* No current dir or parent dir */
        if ( (strcmp(dp -> d_name, ".") == 0) ||
             (strcmp(dp -> d_name, "..") == 0) )
            continue;

        /* Create path string */
        buffer = malloc (sizeof (char) * (PATH_MAX + 1));
        if (!buffer)
        {
            scar_log (1, "%s: Out of memory\n", __func__);

            (void)closedir(dirp);
            free(dp);
            return 1;
        }

        /* Stat the current file */
        res = snprintf (buffer, PATH_MAX, "%s/%s", newrootdir, dp -> d_name);
        if (res >= PATH_MAX)
        {
            scar_log (1, "%s: Out of memory, index not complete.\n", __func__);

            free(buffer);
            continue;
        }

        /* Stat the file */
        if (lstat(buffer, &sb) < 0)
        {
            /* Stat failed */
            free(buffer);
            continue;
        }

        /* Special files */
        if ((S_ISCHR(sb.st_mode)) || (S_ISBLK(sb.st_mode)) || (S_ISFIFO(sb.st_mode)) || (S_ISSOCK (sb.st_mode)))
        {
            /* scar_log (1, "Skipping special file \"%s\"\n", dp -> d_name); */
            free(buffer);
            continue;
        }

        /* Symlink */
        else if (S_ISLNK(sb.st_mode))
        {
            /* scar_log (1, "Skipping symlink \"%s\"\n", dp -> d_name); */
            free(buffer);
            continue;
        }


        /* Directories */
        else if (S_ISDIR(sb.st_mode))
        {
            /* scar_log (1, "Directory \"%s\"\n", dp -> d_name); */
            /* scar_log (1, "Directory \"%s\"\n", buffer); */
            vfs_node = VFS_create_dir (dp -> d_name);
            if (!vfs_node)
                scar_log(1, "Error in VFS_create_dir() for entry \"%s\".\n", dp -> d_name);
            else
            {
                /* Adding current VFS node to tree */
                VFS_add_sibling_to_directory (vfs_parent, vfs_node);

                /* Adding SURL and TURL information to VFS node */
                vfs_node -> surl = createSURL();
                if (vfs_node -> surl)
                {
                    turl = createTURL();
                    setTURLproperties (turl,
                                       buffer,
                                       sb.st_mode,
                                       sb.st_uid,
                                       sb.st_gid,
                                       sb.st_size,
                                       sb.st_blksize,
                                       sb.st_blocks,
                                       sb.st_atime,
                                       sb.st_mtime,
                                       sb.st_ctime);
                    VFS_add_TURL_to_SURL (vfs_node -> surl, turl);
                }

                /* Enter sub-directory */
                walkTheDir (buffer, chrootdir, &(vfs_node -> in_dir));
            }
        }

        /* Regular file */
        else if (S_ISREG(sb.st_mode))
        {
            /* scar_log (1, "Regular file \"%s\"\n", buffer); */
            vfs_node = VFS_create_file (dp -> d_name);
            if (!vfs_node)
                scar_log(1, "Error in VFS_create_file() for entry \"%s\"\n", dp -> d_name);
            else
            {
                /* Adding current VFS node to tree */
                VFS_add_sibling_to_directory (vfs_parent, vfs_node);

                /* Adding SURL and TURL information to VFS node */
                vfs_node -> surl = createSURL();
                if (vfs_node -> surl)
                {
                    turl = createTURL();
                    setTURLproperties (turl,
                                       buffer,
                                       sb.st_mode,
                                       sb.st_uid,
                                       sb.st_gid,
                                       sb.st_size,
                                       sb.st_blksize,
                                       sb.st_blocks,
                                       sb.st_atime,
                                       sb.st_mtime,
                                       sb.st_ctime);
                    VFS_add_TURL_to_SURL (vfs_node -> surl, turl);
                }
            }
        }

        /* Free the buffer */
        free(buffer);
    }

    /* Clean up */
    (void)closedir(dirp);
    free(dp);

    return 0;
}



void * vfs_main (void * arg)
{
    char * rootpath = (char *) arg;
    vfs_t * vfs_root = NULL;

    vfs_root = VFS_create_dir ("/");

    walkTheDir (rootpath, rootpath, &(vfs_root -> in_dir));

    pthread_exit((void *) vfs_root);
}

