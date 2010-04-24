#include "vfs.h"
#include <errno.h>

vfs_t * VFS_path_exists (vfs_t * node, char * path)
{
    char *  tmp_path    = NULL;
    char *  tmp_path2   = NULL;
    char *  a_dir       = NULL;
    int     len         = 0;
    vfs_t * my_node     = NULL;
    int     found       = 0;

    /* Input check */
    if ((path == NULL) || (node == NULL))
    {
        return NULL; /* No such file or directory */
    }

    /* Absolute or relative path equalizer */
    if (path[0] == '/')
    {
        tmp_path = &path[1];
    }

    /* Search for a '/' and count amount of characters for the first directory in the path */
    if ((tmp_path2 = strchr(tmp_path, '/')) == NULL)
    {
        len = strlen(tmp_path) - strlen(tmp_path2);
    }
    else
    {
        len = strlen(tmp_path);
    }


    /* Working with the directory */
    a_dir = malloc (sizeof (char) * (len + 1));

    /* Getting the directory */
    strncpy (a_dir, path, len);

    /* search at VFS node 'current' for a_dir */
    node = my_node;
    while (my_node -> dir_list)
    {
        my_node = my_node -> dir_list;

        /* This makes the VFS case sensitive */
        if (strcmp (my_node -> name, a_dir) == 0)
        {
            /* Found entry */
            found = 1;
            break;
        }
    }

    /* Clear temp data */
    free(a_dir);

    /* The current(ly evaluating) directory didn't hold the element you were looking for */
    if (found == 0)
        return NULL;


    /* If there was a '/' found in path, then advance the tmp_path beyond the slash or we're done */
    if (tmp_path2)
    {
        /* Test for: mydir/ */
        if (tmp_path[1] != '\0')
        {
            tmp_path = &tmp_path[1];

            /* Dive into the next directory */
            return VFS_change_dir (my_node, tmp_path);
        }
        else
        {
            /* Or done */
            return my_node;
        }
    }
    else
    {
        return my_node;
    }
}



vfs_t * VFS_change_dir (vfs_t * current, char * path)
{
    char *  tmp_path    = NULL;
    char *  a_dir       = NULL;
    int     len         = 0;
    vfs_t * changed_dir = NULL;


    /* Input check */
    if ((path == NULL) || (current == NULL))
    {
        return NULL; /* No such file or directory */
    }

    /* Search for a '/' and 
       count amount of characters for the first directory 
       in the path */
    if ((tmp_path = strchr(path, '/')) == NULL)
    {
        len = strlen(path) - strlen(tmp_path);
    }
    else
    {
        len = strlen(path);
    }


    /* Working with the directory */
    a_dir = malloc (sizeof (char) * (len + 1));

    /* Getting the directory */
    strncpy (a_dir, path, len);

    /* search at VFS node 'current' for a_dir */

    /* Clear temp data */
    free(a_dir);


    /* If there was a '/' found in path, then advance the tmp_path beyond the slash or we're done */
    if (tmp_path)
    {
        /* Test for: mydir/ */
        if (tmp_path[1] != '\0')
        {
            tmp_path = &tmp_path[1];

            /* Dive into the next directory */
            return VFS_change_dir (current, tmp_path);
        }
        else
        {
            /* Or done */
            return changed_dir;
        }
    }
    else
    {
        return changed_dir;
    }
}


/* Return file list by searching the VFS on a given path */
char * VFS_list_by_full_path (vfs_t * root, char * path)
{
    vfs_t * node = NULL;
    char * output = NULL;

    if (path == NULL)
    {
        scar_log (1, "%s: Error: No path provided.\n", __func__);
        return NULL;
    }
    if (root == NULL)
    {
        scar_log (1, "%s: Error: No VFS object provided.\n", __func__);
        return NULL;
    }

    /* Output of STAT */
    output = malloc (sizeof (char) * BUF_SIZE_LIST);

    /* Starting part of the STAT return message */
    snprintf (output, BUF_SIZE_LIST - 1, "211- status of %s:\r\n", path); 

    node = root -> in_dir;
    do
    {
        if (node)
            break;

        if (node -> name)
        {
            scar_log (1, "2 ---> node -> name = %s\n", node -> name);
            snprintf (output,
                      BUF_SIZE_LIST - 1, "%s%crwxr-xr-x   1 root users          4096 Nov 22 2009 foo\r\n", 
                      output, 
                      node -> node_type == VFS_DIRECTORY ? 'd' : node -> node_type == VFS_REGULAR_FILE ? '-' : node -> node_type == VFS_SYMLINK ? 'l' : '?',
                      node -> name);
        }

        node = node -> dir_list;
    }
    while (node);

    /* Finalizing list output for STAT output */
    snprintf (output, BUF_SIZE_LIST - 1, "%s211 End of status\r\n", output);

    return output;
}


/* Create a new VFS Directory node */
vfs_t * VFS_create_dir (char * dir_name)
{
    vfs_t * vfs_node = NULL;
    size_t len       = 0;

    if (!dir_name)
        return NULL;

    len = strlen (dir_name);
    if (len == 0)
    {
        scar_log (1, "%s: directory name has zero length. No VFS node created\n", __func__);
        return NULL;
    }
    else if (len > NAME_MAX)
    {
        scar_log (1, "%s: directory name too long. Max length is %d, counted length is %d.\n", __func__, NAME_MAX, len);
        return NULL;
    }

    vfs_node = malloc (sizeof (vfs_t));
    if (!vfs_node)
        return NULL;
    else
    {
        vfs_node -> node_type = VFS_DIRECTORY;
        vfs_node -> name      = strdup (dir_name);
        if (!(vfs_node -> name))
        {
            free(vfs_node);
            return NULL;
        }
        vfs_node -> surl        = NULL;
        vfs_node -> dir_list    = NULL;
        vfs_node -> in_dir      = NULL;

        return vfs_node;
    }
}

/* Create a new VFS Regular file node */
vfs_t * VFS_create_file (char * file_name)
{
    vfs_t * vfs_node = NULL;
    size_t len       = 0;

    if (!file_name)
        return NULL;

    len = strlen(file_name);
    if (len == 0)
    {
        scar_log (1, "%s: file name has zero length. No VFS node created\n", __func__);
        return NULL;
    }
    else if (len > NAME_MAX)
    {
        scar_log (1, "%s: file name too long. Max length is %d, counted length is %d.\n", __func__, NAME_MAX, len);
        return NULL;
    }

    vfs_node = malloc (sizeof (vfs_t));
    if (!vfs_node)
        return NULL;
    else
    {
        vfs_node -> node_type = VFS_REGULAR_FILE;
        vfs_node -> name      = strdup (file_name);
        if (!(vfs_node -> name))
        {
            free(vfs_node);
            return NULL;
        }
        vfs_node -> surl        = NULL;
        vfs_node -> dir_list    = NULL;
        vfs_node -> in_dir      = NULL;

        return vfs_node;
    }
}



/* Adding sibling VFS node to a dir VFS */
int VFS_add_sibling_to_directory (vfs_t ** list, vfs_t * entry)
{
    vfs_t * tmp_list = *list;

    if (!list)
    {
        scar_log (1, "%s: no directory vfs_t pointer provided\n", __func__); 
        return 1;
    }
    if (!entry)
    {
        scar_log (1, "%s: no entry vfs_t provided\n", __func__); 
        return 1;
    }

    if (*list == NULL)
    {
        *list = entry;
        return 0;
    }
    else
    {
        while (tmp_list -> dir_list)
        {
            tmp_list = tmp_list -> dir_list;
        }
        tmp_list -> dir_list = entry;
        return 0;
    }
}

void VFS_print_real (vfs_t * vfs_node, int * indent)
{
    vfs_t *   curr = vfs_node;
    turl_t *  turl = NULL;
    char *    padl = NULL;

    padl = malloc (sizeof(char) * *indent + 1);
    memset (padl, ' ', *indent);
    padl[*indent] = '\0';

    while (curr)
    {
        if (curr -> node_type == VFS_DIRECTORY)
        {
            scar_log (1, "%sD: %s\n", padl, curr -> name);
            if (curr -> in_dir)
            {
                (*indent)++;
                VFS_print_real (curr -> in_dir, indent);
                (*indent)--;
            }
        }
        else if (curr -> node_type == VFS_REGULAR_FILE)
        {
            scar_log (1, "%sF: %s\n", padl, curr -> name);
        }
        else if (curr -> node_type == VFS_SYMLINK)
        {
            scar_log (1, "%sL: %s\n", padl, curr -> name);
        }

        if (curr -> surl)
        {
            scar_log (1, "%s\t- SURL:\n", padl);
            scar_log (1, "%s\tnlinks           : %7d\n", padl, (int) curr -> surl -> nlink); 
            scar_log (1, "%s\tuse_counter      : %7d\n", padl, (int) curr -> surl -> use_counter); 
            scar_log (1, "%s\thas a filehash   : %s\n", padl, curr -> surl -> filehash_list ? "yes" : "no"); 
            scar_log (1, "%s\thas an ACL       : %s\n", padl, curr -> surl -> acls ? "yes" : "no"); 

            if (curr -> surl -> turl_list)
            {
                turl = curr -> surl -> turl_list;

                while (turl)
                {
                    scar_log (1, "%s\t\t- TURL:\n", padl);
                    scar_log (1, "%s\t\ttimestamp        : %7d\n", padl, turl -> timestamp);
                    scar_log (1, "%s\t\thas an protocol  : %s\n", padl, turl -> protocol ? "yes" : "no");
                    scar_log (1, "%s\t\tpath             : %s\n", padl, turl -> path);
                    scar_log (1, "%s\t\thas a filehash   : %s\n", padl, turl -> filehash_list ? "yes" : "no");
                    scar_log (1, "%s\t\tusage counter    : %7d\n", padl, turl -> use_counter);
                    scar_log (1, "%s\t\tmode             : %7d\n", padl, turl -> mode);
                    scar_log (1, "%s\t\tuid              : %7d\n", padl, turl -> uid);
                    scar_log (1, "%s\t\tgid              : %7d\n", padl, turl -> gid);
                    scar_log (1, "%s\t\tsize             : %7d\n", padl, turl -> size);
                    scar_log (1, "%s\t\tblksize          : %7d\n", padl, turl -> blksize);
                    scar_log (1, "%s\t\tblocks           : %7d\n", padl, turl -> blocks);
                    scar_log (1, "%s\t\tatime            : %7d\n", padl, turl -> atime);
                    scar_log (1, "%s\t\tmtime            : %7d\n", padl, turl -> mtime);
                    scar_log (1, "%s\t\tctime            : %7d\n", padl, turl -> ctime);

                    turl = turl -> next;
                }
            }
        }

        /* Go to next element */
        curr = curr -> dir_list;
    }
    free(padl);
}

void VFS_print (vfs_t * vfs_node)
{
    int i = 0;
    VFS_print_real (vfs_node, &i);
}

turl_t * createTURL (void)
{
    turl_t * turl = NULL;
    turl = malloc (sizeof (turl_t));
    if (!turl) 
        scar_log (1, "%s: Out of memory\n", __func__);
    else
        memset (turl, 0, sizeof(turl_t));

    return turl;
}

/*
int setTURLprotocol   (turl_t *   turl,
                       char * protocol_name,
                       is_active,
                       hostname,
                       port)
{
    if (!turl)
    {
        scar_log (1, "%s: No TURL object provided to function\n", __func__);
        return 1;
    }

    turl -> protocol protocol_name = protocol_name;
    turl -> is_active     = is_active;
    turl -> hostname      = hostname;
    turl -> port          = port;

    return 0;
}
*/


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
                       time_t     p_ctime)
{
    char * path = NULL;

    if (strlen(absolute_path) > PATH_MAX)
        return 1;

    if (!turl)
    {
        scar_log (1, "%s: No TURL object provided to function\n", __func__);
        return 1;
    }

    path = strdup(absolute_path);
    if (!path)
    {
        scar_log (1, "%s: Out of memory\n", __func__);
        return 1;
    }

    turl -> timestamp     = time(NULL);
    turl -> protocol      = NULL;
    turl -> path          = path;
    turl -> filehash_list = NULL;
    turl -> use_counter   = 0;
    turl -> mode          = mode;       
    turl -> uid           = uid;        
    turl -> gid           = gid;        
    turl -> size          = size;       
    turl -> blksize       = blksize;    
    turl -> blocks        = blocks;     
    turl -> atime         = p_atime;      
    turl -> mtime         = p_mtime;      
    turl -> ctime         = p_ctime;      
    turl -> next          = NULL;       

    return 0;
}

/* Adding TURL to a SURL */
int VFS_add_TURL_to_SURL (surl_t * surl, turl_t * turl)
{
    turl_t * tmp = NULL;

    if (!surl)
    {
        scar_log (1, "%s: no SURL object presented to function\n", __func__); 
        return 1;
    }
    if (!turl)
    {
        scar_log (1, "%s: no TURL object presented to function\n", __func__); 
        return 1;
    }

    if (surl -> turl_list == NULL)
    {
        surl -> turl_list = turl;
        surl -> nlink++;
        return 0;
    }
    else
    {
        tmp = surl -> turl_list;
        while (tmp -> next)
        {
            tmp = tmp -> next;
        }
        tmp -> next = turl;
        surl -> nlink++;
        return 0;
    }
}

surl_t * createSURL (void)
{
    surl_t * surl = NULL;
    surl = malloc (sizeof (surl_t));
    if (!surl) 
        scar_log (1, "%s: Out of memory\n", __func__);
    else
        memset (surl, 0, sizeof(surl_t));

    return surl;
}


/* Adding SURL to a VFS node */
int VFS_add_SURL_to_VFS (vfs_t * entry, surl_t * surl)
{
    if (!entry)
    {
        scar_log (1, "%s: no VFS object presented to function\n", __func__); 
        return 1;
    }
    if (!surl)
    {
        scar_log (1, "%s: no SURL object presented to function\n", __func__); 
        return 1;
    }

    if (entry -> surl != NULL)
    {
        scar_log (1, "%s: VFS object already has a SURL, skipping linkage\n", __func__); 
        return 1;
    }
    else
    {
        entry -> surl = surl;
        return 0;
    }
}
