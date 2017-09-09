// platform specific (linux x86 64-bit)

typedef unsigned int mode_t;
typedef unsigned long dev_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef long off_t;

void *memcpy(void *dest, const void *src, size_t n);

struct timespec {
	long	tv_sec;			/* seconds */
	long	tv_nsec;		/* nanoseconds */
};

struct stat {
    unsigned long st_dev;
    unsigned long st_ino;
    unsigned long st_nlink;
    unsigned int  st_mode;
    unsigned int  st_uid;
    unsigned int  st_gid;
    int __pad0;
    unsigned long st_rdev;
    long		  st_size;
    long		  st_blksize;
    long		  st_blocks;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    long		 __glibc_reserved[3];
};

struct statvfs {
	unsigned long	f_bsize;    /* Filesystem block size */
	unsigned long	f_frsize;   /* Fragment size */
	unsigned long	f_blocks;   /* Size of fs in f_frsize units */
	unsigned long	f_bfree;    /* Number of free blocks */
	unsigned long 	f_bavail;   /* Number of free blocks for
                                             unprivileged users */
	unsigned long	f_files;    /* Number of inodes */
	unsigned long	f_ffree;    /* Number of free inodes */
	unsigned long	f_favail;   /* Number of free inodes for
                                             unprivileged users */
	unsigned long 	f_fsid;     /* Filesystem ID */
	unsigned long 	f_flag;     /* Mount flags */
	unsigned long	f_namemax;  /* Maximum filename length */
};
