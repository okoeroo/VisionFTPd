#ifndef INDEX_DATAMODEL_H
    #define INDEX_DATAMODEL_H

/* File Hash */
typedef struct filehash_s {
    char * method;
    char * hash_data;
    time_t timeofhash;
} filehash_t;


/* List of File Hashes */
typedef struct filehash_list_s {
    filehash_t filehash;
    struct filehash_list_s * next;
} filehash_list_t;


typedef enum acl_rule_type_e {
    POSIX
} acl_rule_type_t;

/* VFS ACL */
typedef struct vfs_acl_s {
    mode_t st_mode;
    acl_rule_type_t rule_types;
} acl_t;


/* (Access) Protocol specifications */
typedef struct protocol_s {
    char * protocol_name;  /* Name of a (supported) protocol */
    int    is_active;      /* When 0 this means it's not active and must be reached directly
                            * When !0 this means it's actively connecting to a client. */
    unsigned char * hostname;  /* Hostname or IP to connect to, only needed in non-active mode */
    unsigned short port;       /* Port number to connect to */
} protocol_t;


/* Transport URL - many per site/cluster, but one per file on host per protocol */
typedef struct turl_s {
    time_t            record_time;     /* States the creation or updated time of this record */
    protocol_t *      protocol;        /* Specifies access protocol details for this file */
    char *            path;            /* Abolute path to the file on the Data-Mover */
    filehash_list_t * filehash_list;   /* The current hash value of this file */
    int               use_counter;     /* This is to keep track of the usage of this file. Needed for hot-file counter */
    mode_t            mode;            /* protection */
    uid_t             uid;             /* user ID of owner */
    gid_t             gid;             /* group ID of owner */
    off_t             size;            /* total size, in bytes */
    blksize_t         blksize;         /* blocksize for filesystem I/O */
    blkcnt_t          blocks;          /* number of blocks allocated */
    time_t            atime;           /* time of last access */
    time_t            mtime;           /* time of last modification */
    time_t            ctime;           /* time of last status change */
    struct turl_s *   next;            /* Next TURL in SURL or TURL list */
} turl_t;


/* List of Transport URLs - This is the highest structure pushed from a Data-Mover to a Commander */
typedef struct turl_list_s {
    turl_t * turl;
    struct turl_list_s *next;
} turl_list_t;


/* Storage URL - 1 file can be accesible at multiple TURLs */
typedef struct surl_s {
    nlink_t           nlink;         /* Number of available TURL links */
    turl_t *          turl_list;     /* List of TURLs to access a file from a Data-Mover */
    int               use_counter;   /* Usage counter to signal hot files */
    filehash_list_t * filehash_list; /* Cluster wide file hashes */
    acl_t *           acls;
} surl_t;


/* Virtual File System - file types */
typedef enum vfs_node_e {
    VFS_DIRECTORY,
    VFS_REGULAR_FILE,
    VFS_SYMLINK,
} vfs_node_t;


/* Virtual File System - directory struture */
typedef struct vfs_s {
    vfs_node_t       node_type;   /* What type of VFS record does this describe */
    char *           name;        /* Logical (File) Name in the VFS */
    surl_t        *  surl;        /* The Storage URL - Only used for vfs_node_t VFS_REGULAR_FILE */
    struct vfs_s  *  dir_list;    /* Next VFS node in directory */
    struct vfs_s  *  in_dir;      /* First entry in the directory */
    struct vfs_s  *  symlink;     /* Symlink to other VFS node */
} vfs_t;


#endif /* INDEX_DATAMODEL_H */
