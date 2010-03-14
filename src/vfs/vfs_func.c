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
        vfs_node -> child_nodes = NULL;

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
        vfs_node -> child_nodes = NULL;

        return vfs_node;
    }
}


/* void VFS_print_real (vfs_t * vfs_node, int indent) */
void VFS_print (vfs_t * vfs_node)
{
    if (vfs_node)
    {
        if (vfs_node -> node_type == VFS_DIRECTORY)
            scar_log (1, "D: %s\n", vfs_node -> name);
        else if (vfs_node -> node_type == VFS_REGULAR_FILE)
            scar_log (1, "F: %s\n", vfs_node -> name);
        else if (vfs_node -> node_type == VFS_SYMLINK)
            scar_log (1, "L: %s\n", vfs_node -> name);


        if (vfs_node -> child_nodes)
            VFS_print (vfs_node -> child_nodes);
            /* VFS_print_real (vfs_node -> child_nodes, ind); */
    }
}


/* Adding a child VFS node to a parent VFS */
int VFS_add_child_to_parent (vfs_t * parent, vfs_t * child)
{
    vfs_t * tmp_parent = NULL;

    if (!parent)
    {
        scar_log (1, "%s: no parent vfs_t provided\n", __func__); 
        return 1;
    }
    if (!child)
    {
        scar_log (1, "%s: no child vfs_t provided\n", __func__); 
        return 1;
    }

    /*
    scar_log (1, "At %s adding %s %s\n", parent -> name,
                                         child -> node_type == VFS_DIRECTORY ? "DIR " : "FILE",
                                         child -> name);
    */

    /* Fast-Forward to last child_nodes */
    tmp_parent = parent;
    if (tmp_parent -> child_nodes == NULL)
        tmp_parent -> child_nodes = child;
    else
    {
        while (tmp_parent -> child_nodes)
        {
            tmp_parent = tmp_parent -> child_nodes;
        }
        tmp_parent -> child_nodes = child;
    }

    return 0;
}
