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


#endif /* VFS_H */
