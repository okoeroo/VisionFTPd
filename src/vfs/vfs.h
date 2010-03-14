#include "main.h"
#include "vfs_dm.h"

#include <dirent.h>
#include <sys/stat.h>


#ifndef VFS_H
    #define VFS_H

/* Walk through the directory */
int walkTheDir (char * newrootdir, char * chrootdir, vfs_t * vfs_parent);

/* Create a new VFS Directory node */
vfs_t * VFS_create_dir (char * dir_name);

/* Create a new VFS Regular file node */
vfs_t * VFS_create_file (char * file_name);

/* Adding a child VFS node to a parent VFS */
int VFS_add_child_to_parent (vfs_t * parent, vfs_t * child);

void VFS_print (vfs_t * vfs_node);

/* Main indexing function */
void * vfs_main (void * rootpath);


#endif /* VFS_H */
