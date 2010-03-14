#include "vfs.h"



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
    vfs_t * curr = vfs_node;
    char * padl  = NULL;

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
        curr = curr -> dir_list;
    }
    free(padl);
}

void VFS_print (vfs_t * vfs_node)
{
    int i = 0;
    VFS_print_real (vfs_node, &i);
}

