#ifndef INDEX_DATAMODEL_H
    #define INDEX_DATAMODEL_H

/* File Hash */
typedef struct filehash_s {
    char * method;
    char * hash_data;
    time_t timeofhash;
} filehash_t;

/* Transport URL - many per site/cluster, but one per file on host per protocol */
typedef struct turl_s {
    char *          protocol;
    char *          host;
    char *          filesystem;
    char *          path;
    long            size;
    time_t          ctime;
    time_t          atime;
    time_t          mtime;
    filehash_t      filehash;
    int             use_counter;
    struct turl_s * next;
} turl_t;

/* Storage URL - 1 per site/cluster */
typedef struct surl_s {
    int             local_replica_count;
    turl_t *        turls;
    char *          host;
    int             port;
    struct surl_s * next;
    int             use_counter;
} surl_t;

/* Globally Unique Identifier */
typedef struct file_guid_s {
    long       guid;
    surl_t *   surls;
    filehash_t filehash;
    acl_t *    acls;
} file_guid_t;

/* Logical File Name */
typedef struct lfn_s {
    char *        name;
    file_guid_t * file_guid;
} lfn_t;

/* Virtual File System - file types */
typedef enum vfs_node_e {
    VFS_DIRECTORY;
    VFS_FILE;
} vfsnode_t;

/* Virtual File System - directory struture */
typedef struct vfs_s {
    lfn_t *          lfn;
    vfsnode_t        node_type;
    struct vfs_s *   list;
} vfs_t;


#endif /* INDEX_DATAMODEL_H */
