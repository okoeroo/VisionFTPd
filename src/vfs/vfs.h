#include "main.h"
#include "vfs_dm.h"

#include <dirent.h>
#include <sys/stat.h>


#ifndef VFS_H
    #define VFS_H

/* Walk through the directory */
int walkTheDir (char * newrootdir, char * chrootdir, vfs_t ** vfs_parent);

/* Create a new VFS Directory node */
vfs_t * VFS_create_dir (char * dir_name);

/* Create a new VFS Regular file node */
vfs_t * VFS_create_file (char * file_name);

/* Adding sibling VFS node to a dir VFS */
int VFS_add_sibling_to_directory (vfs_t ** list, vfs_t * entry);

/* Print the VFS tree */
void VFS_print_real (vfs_t * vfs_node, int * indent);
void VFS_print (vfs_t * vfs_node);

/* Main indexing function */
void * vfs_main (void * rootpath);



int setTURLproperties (turl_t *   turl,
                       char *     absolute_path,
                       mode_t     mode,
                       uid_t      uid,
                       gid_t      gid,
                       off_t      size,
                       blksize_t  blksize,
                       blkcnt_t   blocks,
                       time_t     p_atime,
                       time_t     p_mtime,
                       time_t     p_ctime);

/* Adding TURL to a SURL */
int VFS_add_TURL_to_SURL (surl_t * surl, turl_t * turl);

turl_t * createTURL (void);
surl_t * createSURL (void);

/* Adding SURL to a VFS node */
int VFS_add_SURL_to_VFS (vfs_t * entry, surl_t * surl);


/* Virtual VFS unmarshall from transport */
vfs_t * VFS_unmarshall (unsigned char * blob);

/* Virtual VFS marshall for transport */
unsigned char * VFS_marshall (vfs_t * vfs);

#endif /* VFS_H */
