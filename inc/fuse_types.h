// types for fuse3
int fuse_main_real(int argc, char *argv[], 
	const struct fuse_operations *op, size_t op_size, void *private_data);
	
/** Function to add an entry in a readdir() operation
 *
 * The *off* parameter can be any non-zero value that enableds the
 * filesystem to identify the current point in the directory
 * stream. It does not need to be the actual physical position. A
 * value of zero is reserved to indicate that seeking in directories
 * is not supported.
 * 
 * @param buf the buffer passed to the readdir() operation
 * @param name the file name of the directory entry
 * @param stat file attributes, can be NULL
 * @param off offset of the next entry or zero
 * @param flags fill flags
 * @return 1 if buffer is full, zero otherwise
 */
typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
				const struct stat *stbuf, off_t off,
				enum fuse_fill_dir_flags flags);

/**
 * The file system operations:
 *
 * Most of these should work very similarly to the well known UNIX
 * file system operations.  A major exception is that instead of
 * returning an error in 'errno', the operation should return the
 * negated error value (-errno) directly.
 *
 * All methods are optional, but some are essential for a useful
 * filesystem (e.g. getattr).  Open, flush, release, fsync, opendir,
 * releasedir, fsyncdir, access, create, truncate, lock, init and
 * destroy are special purpose methods, without which a full featured
 * filesystem can still be implemented.
 *
 * In general, all methods are expected to perform any necessary
 * permission checking. However, a filesystem may delegate this task
 * to the kernel by passing the `default_permissions` mount option to
 * `fuse_new()`. In this case, methods will only be called if
 * the kernel's permission check has succeeded.
 *
 * Almost all operations take a path which can be of any length.
 */
struct fuse_operations {
	/** Get file attributes.
	 *
	 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
	 * ignored. The 'st_ino' field is ignored except if the 'use_ino'
	 * mount option is given. In that case it is passed to userspace,
	 * but libfuse and the kernel will still assign a different
	 * inode for internal use (called the "nodeid").
	 *
	 * `fi` will always be NULL if the file is not currenly open, but
	 * may also be NULL if the file is open.
	 */
	int (*getattr) (const char *, struct stat *, struct fuse_file_info *fi);

	/** Read the target of a symbolic link
	 *
	 * The buffer should be filled with a null terminated string.  The
	 * buffer size argument includes the space for the terminating
	 * null character.	If the linkname is too long to fit in the
	 * buffer, it should be truncated.	The return value should be 0
	 * for success.
	 */
	int (*readlink) (const char *, char *, size_t);

	/** Create a file node
	 *
	 * This is called for creation of all non-directory, non-symlink
	 * nodes.  If the filesystem defines a create() method, then for
	 * regular files that will be called instead.
	 */
	int (*mknod) (const char *, mode_t, dev_t);

	/** Create a directory
	 *
	 * Note that the mode argument may not have the type specification
	 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
	 * correct directory type bits use  mode|S_IFDIR
	 * */
	int (*mkdir) (const char *, mode_t);

	/** Remove a file */
	int (*unlink) (const char *);

	/** Remove a directory */
	int (*rmdir) (const char *);

	/** Create a symbolic link */
	int (*symlink) (const char *, const char *);

	/** Rename a file
	 *
	 * *flags* may be `RENAME_EXCHANGE` or `RENAME_NOREPLACE`. If
	 * RENAME_NOREPLACE is specified, the filesystem must not
	 * overwrite *newname* if it exists and return an error
	 * instead. If `RENAME_EXCHANGE` is specified, the filesystem
	 * must atomically exchange the two files, i.e. both must
	 * exist and neither may be deleted.
	 */
	int (*rename) (const char *, const char *, unsigned int flags);

	/** Create a hard link to a file */
	int (*link) (const char *, const char *);

	/** Change the permission bits of a file
	 *
	 * `fi` will always be NULL if the file is not currenly open, but
	 * may also be NULL if the file is open.
	 */
	int (*chmod) (const char *, mode_t, struct fuse_file_info *fi);

	/** Change the owner and group of a file
	 *
	 * `fi` will always be NULL if the file is not currenly open, but
	 * may also be NULL if the file is open.
	 *
	 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
	 * expected to reset the setuid and setgid bits.
	 */
	int (*chown) (const char *, uid_t, gid_t, struct fuse_file_info *fi);

	/** Change the size of a file
	 *
	 * `fi` will always be NULL if the file is not currenly open, but
	 * may also be NULL if the file is open.
	 *
	 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
	 * expected to reset the setuid and setgid bits.
	 */
	int (*truncate) (const char *, off_t, struct fuse_file_info *fi);

	/** Open a file
	 *
	 * Open flags are available in fi->flags. The following rules
	 * apply.
	 *
	 *  - Creation (O_CREAT, O_EXCL, O_NOCTTY) flags will be
	 *    filtered out / handled by the kernel.
	 *
	 *  - Access modes (O_RDONLY, O_WRONLY, O_RDWR) should be used
	 *    by the filesystem to check if the operation is
	 *    permitted.  If the ``-o default_permissions`` mount
	 *    option is given, this check is already done by the
	 *    kernel before calling open() and may thus be omitted by
	 *    the filesystem.
	 *
	 *  - When writeback caching is enabled, the kernel may send
	 *    read requests even for files opened with O_WRONLY. The
	 *    filesystem should be prepared to handle this.
	 *
	 *  - When writeback caching is disabled, the filesystem is
	 *    expected to properly handle the O_APPEND flag and ensure
	 *    that each write is appending to the end of the file.
	 * 
         *  - When writeback caching is enabled, the kernel will
	 *    handle O_APPEND. However, unless all changes to the file
	 *    come through the kernel this will not work reliably. The
	 *    filesystem should thus either ignore the O_APPEND flag
	 *    (and let the kernel handle it), or return an error
	 *    (indicating that reliably O_APPEND is not available).
	 *
	 * Filesystem may store an arbitrary file handle (pointer,
	 * index, etc) in fi->fh, and use this in other all other file
	 * operations (read, write, flush, release, fsync).
	 *
	 * Filesystem may also implement stateless file I/O and not store
	 * anything in fi->fh.
	 *
	 * There are also some flags (direct_io, keep_cache) which the
	 * filesystem may set in fi, to change the way the file is opened.
	 * See fuse_file_info structure in <fuse_common.h> for more details.
	 *
	 * If this request is answered with an error code of ENOSYS
	 * and FUSE_CAP_NO_OPEN_SUPPORT is set in
	 * `fuse_conn_info.capable`, this is treated as success and
	 * future calls to open will also succeed without being send
	 * to the filesystem process.
	 *
	 */
	int (*open) (const char *, struct fuse_file_info *);

	/** Read data from an open file
	 *
	 * Read should return exactly the number of bytes requested except
	 * on EOF or error, otherwise the rest of the data will be
	 * substituted with zeroes.	 An exception to this is when the
	 * 'direct_io' mount option is specified, in which case the return
	 * value of the read system call will reflect the return value of
	 * this operation.
	 */
	int (*read) (const char *, char *, size_t, off_t,
		     struct fuse_file_info *);

	/** Write data to an open file
	 *
	 * Write should return exactly the number of bytes requested
	 * except on error.	 An exception to this is when the 'direct_io'
	 * mount option is specified (see read operation).
	 *
	 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
	 * expected to reset the setuid and setgid bits.
	 */
	int (*write) (const char *, const char *, size_t, off_t,
		      struct fuse_file_info *);

	/** Get file system statistics
	 *
	 * The 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
	 */
	int (*statfs) (const char *, struct statvfs *);

	/** Possibly flush cached data
	 *
	 * BIG NOTE: This is not equivalent to fsync().  It's not a
	 * request to sync dirty data.
	 *
	 * Flush is called on each close() of a file descriptor.  So if a
	 * filesystem wants to return write errors in close() and the file
	 * has cached dirty data, this is a good place to write back data
	 * and return any errors.  Since many applications ignore close()
	 * errors this is not always useful.
	 *
	 * NOTE: The flush() method may be called more than once for each
	 * open().	This happens if more than one file descriptor refers
	 * to an opened file due to dup(), dup2() or fork() calls.	It is
	 * not possible to determine if a flush is final, so each flush
	 * should be treated equally.  Multiple write-flush sequences are
	 * relatively rare, so this shouldn't be a problem.
	 *
	 * Filesystems shouldn't assume that flush will always be called
	 * after some writes, or that if will be called at all.
	 */
	int (*flush) (const char *, struct fuse_file_info *);

	/** Release an open file
	 *
	 * Release is called when there are no more references to an open
	 * file: all file descriptors are closed and all memory mappings
	 * are unmapped.
	 *
	 * For every open() call there will be exactly one release() call
	 * with the same flags and file descriptor.	 It is possible to
	 * have a file opened more than once, in which case only the last
	 * release will mean, that no more reads/writes will happen on the
	 * file.  The return value of release is ignored.
	 */
	int (*release) (const char *, struct fuse_file_info *);

	/** Synchronize file contents
	 *
	 * If the datasync parameter is non-zero, then only the user data
	 * should be flushed, not the meta data.
	 */
	int (*fsync) (const char *, int, struct fuse_file_info *);

	/** Set extended attributes */
	int (*setxattr) (const char *, const char *, const char *, size_t, int);

	/** Get extended attributes */
	int (*getxattr) (const char *, const char *, char *, size_t);

	/** List extended attributes */
	int (*listxattr) (const char *, char *, size_t);

	/** Remove extended attributes */
	int (*removexattr) (const char *, const char *);

	/** Open directory
	 *
	 * Unless the 'default_permissions' mount option is given,
	 * this method should check if opendir is permitted for this
	 * directory. Optionally opendir may also return an arbitrary
	 * filehandle in the fuse_file_info structure, which will be
	 * passed to readdir, closedir and fsyncdir.
	 */
	int (*opendir) (const char *, struct fuse_file_info *);

	/** Read directory
	 *
	 * The filesystem may choose between two modes of operation:
	 *
	 * 1) The readdir implementation ignores the offset parameter, and
	 * passes zero to the filler function's offset.  The filler
	 * function will not return '1' (unless an error happens), so the
	 * whole directory is read in a single readdir operation.
	 *
	 * 2) The readdir implementation keeps track of the offsets of the
	 * directory entries.  It uses the offset parameter and always
	 * passes non-zero offset to the filler function.  When the buffer
	 * is full (or an error happens) the filler function will return
	 * '1'.
	 */
	int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t,
			struct fuse_file_info *, enum fuse_readdir_flags);

	/** Release directory
	 */
	int (*releasedir) (const char *, struct fuse_file_info *);

	/** Synchronize directory contents
	 *
	 * If the datasync parameter is non-zero, then only the user data
	 * should be flushed, not the meta data
	 */
	int (*fsyncdir) (const char *, int, struct fuse_file_info *);

	/**
	 * Initialize filesystem
	 *
	 * The return value will passed in the `private_data` field of
	 * `struct fuse_context` to all file operations, and as a
	 * parameter to the destroy() method. It overrides the initial
	 * value provided to fuse_main() / fuse_new().
	 */
	void *(*init) (struct fuse_conn_info *conn,
		       struct fuse_config *cfg);

	/**
	 * Clean up filesystem
	 *
	 * Called on filesystem exit.
	 */
	void (*destroy) (void *private_data);

	/**
	 * Check file access permissions
	 *
	 * This will be called for the access() system call.  If the
	 * 'default_permissions' mount option is given, this method is not
	 * called.
	 *
	 * This method is not called under Linux kernel versions 2.4.x
	 */
	int (*access) (const char *, int);

	/**
	 * Create and open a file
	 *
	 * If the file does not exist, first create it with the specified
	 * mode, and then open it.
	 *
	 * If this method is not implemented or under Linux kernel
	 * versions earlier than 2.6.15, the mknod() and open() methods
	 * will be called instead.
	 */
	int (*create) (const char *, mode_t, struct fuse_file_info *);

	/**
	 * Perform POSIX file locking operation
	 *
	 * The cmd argument will be either F_GETLK, F_SETLK or F_SETLKW.
	 *
	 * For the meaning of fields in 'struct flock' see the man page
	 * for fcntl(2).  The l_whence field will always be set to
	 * SEEK_SET.
	 *
	 * For checking lock ownership, the 'fuse_file_info->owner'
	 * argument must be used.
	 *
	 * For F_GETLK operation, the library will first check currently
	 * held locks, and if a conflicting lock is found it will return
	 * information without calling this method.	 This ensures, that
	 * for local locks the l_pid field is correctly filled in.	The
	 * results may not be accurate in case of race conditions and in
	 * the presence of hard links, but it's unlikely that an
	 * application would rely on accurate GETLK results in these
	 * cases.  If a conflicting lock is not found, this method will be
	 * called, and the filesystem may fill out l_pid by a meaningful
	 * value, or it may leave this field zero.
	 *
	 * For F_SETLK and F_SETLKW the l_pid field will be set to the pid
	 * of the process performing the locking operation.
	 *
	 * Note: if this method is not implemented, the kernel will still
	 * allow file locking to work locally.  Hence it is only
	 * interesting for network filesystems and similar.
	 */
	int (*lock) (const char *, struct fuse_file_info *, int cmd,
		     struct flock *);

	/**
	 * Change the access and modification times of a file with
	 * nanosecond resolution
	 *
	 * This supersedes the old utime() interface.  New applications
	 * should use this.
	 *
	 * `fi` will always be NULL if the file is not currenly open, but
	 * may also be NULL if the file is open.
	 *
	 * See the utimensat(2) man page for details.
	 */
	 int (*utimens) (const char *, const struct timespec tv[2],
			 struct fuse_file_info *fi);

	/**
	 * Map block index within file to block index within device
	 *
	 * Note: This makes sense only for block device backed filesystems
	 * mounted with the 'blkdev' option
	 */
	int (*bmap) (const char *, size_t blocksize, uint64_t *idx);

	/**
	 * Ioctl
	 *
	 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
	 * 64bit environment.  The size and direction of data is
	 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
	 * data will be NULL, for _IOC_WRITE data is out area, for
	 * _IOC_READ in area and if both are set in/out area.  In all
	 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
	 *
	 * If flags has FUSE_IOCTL_DIR then the fuse_file_info refers to a
	 * directory file handle.
	 */
	int (*ioctl) (const char *, int cmd, void *arg,
		      struct fuse_file_info *, unsigned int flags, void *data);

	/**
	 * Poll for IO readiness events
	 *
	 * Note: If ph is non-NULL, the client should notify
	 * when IO readiness events occur by calling
	 * fuse_notify_poll() with the specified ph.
	 *
	 * Regardless of the number of times poll with a non-NULL ph
	 * is received, single notification is enough to clear all.
	 * Notifying more times incurs overhead but doesn't harm
	 * correctness.
	 *
	 * The callee is responsible for destroying ph with
	 * fuse_pollhandle_destroy() when no longer in use.
	 */
	int (*poll) (const char *, struct fuse_file_info *,
		     struct fuse_pollhandle *ph, unsigned *reventsp);

	/** Write contents of buffer to an open file
	 *
	 * Similar to the write() method, but data is supplied in a
	 * generic buffer.  Use fuse_buf_copy() to transfer data to
	 * the destination.
	 *
	 * Unless FUSE_CAP_HANDLE_KILLPRIV is disabled, this method is
	 * expected to reset the setuid and setgid bits.
	 */
	int (*write_buf) (const char *, struct fuse_bufvec *buf, off_t off,
			  struct fuse_file_info *);

	/** Store data from an open file in a buffer
	 *
	 * Similar to the read() method, but data is stored and
	 * returned in a generic buffer.
	 *
	 * No actual copying of data has to take place, the source
	 * file descriptor may simply be stored in the buffer for
	 * later data transfer.
	 *
	 * The buffer must be allocated dynamically and stored at the
	 * location pointed to by bufp.  If the buffer contains memory
	 * regions, they too must be allocated using malloc().  The
	 * allocated memory will be freed by the caller.
	 */
	int (*read_buf) (const char *, struct fuse_bufvec **bufp,
			 size_t size, off_t off, struct fuse_file_info *);
	/**
	 * Perform BSD file locking operation
	 *
	 * The op argument will be either LOCK_SH, LOCK_EX or LOCK_UN
	 *
	 * Nonblocking requests will be indicated by ORing LOCK_NB to
	 * the above operations
	 *
	 * For more information see the flock(2) manual page.
	 *
	 * Additionally fi->owner will be set to a value unique to
	 * this open file.  This same value will be supplied to
	 * ->release() when the file is released.
	 *
	 * Note: if this method is not implemented, the kernel will still
	 * allow file locking to work locally.  Hence it is only
	 * interesting for network filesystems and similar.
	 */
	int (*flock) (const char *, struct fuse_file_info *, int op);

	/**
	 * Allocates space for an open file
	 *
	 * This function ensures that required space is allocated for specified
	 * file.  If this function returns success then any subsequent write
	 * request to specified range is guaranteed not to fail because of lack
	 * of space on the file system media.
	 */
	int (*fallocate) (const char *, int, off_t, off_t,
			  struct fuse_file_info *);
};

/**
 * Information about open files
 */
struct fuse_file_info {
	/** Open flags.	 Available in open() and release() */
	int flags;

	/** In case of a write operation indicates if this was caused by a
	    writepage */
	unsigned int writepage : 1;

	/** Can be filled in by open, to use direct I/O on this file. */
	unsigned int direct_io : 1;

	/** Can be filled in by open, to indicate that currently
	    cached file data (that the filesystem provided the last
	    time the file was open) need not be invalidated. */
	unsigned int keep_cache : 1;

	/** Indicates a flush operation.  Set in flush operation, also
	    maybe set in highlevel lock operation and lowlevel release
	    operation. */
	unsigned int flush : 1;

	/** Can be filled in by open, to indicate that the file is not
	    seekable. */
	unsigned int nonseekable : 1;

	/* Indicates that flock locks for this file should be
	   released.  If set, lock_owner shall contain a valid value.
	   May only be set in ->release(). */
	unsigned int flock_release : 1;

	/** Padding.  Do not use*/
	unsigned int padding : 27;

	/** File handle.  May be filled in by filesystem in open().
	    Available in all other file operations */
	uint64_t fh;

	/** Lock owner id.  Available in locking operations and flush */
	uint64_t lock_owner;

	/** Requested poll events.  Available in ->poll.  Only set on kernels
	    which support it.  If unsupported, this field is set to zero. */
	uint32_t poll_events;
};

/**
 * Configuration of the high-level API
 *
 * This structure is initialized from the arguments passed to
 * fuse_new(), and then passed to the file system's init() handler
 * which should ensure that the configuration is compatible with the
 * file system implementation.
 */
struct fuse_config {
	/**
	 * If `set_gid` is non-zero, the st_gid attribute of each file
	 * is overwritten with the value of `gid`.
	 */
	int set_gid;
	unsigned int gid;

	/**
	 * If `set_uid` is non-zero, the st_uid attribute of each file
	 * is overwritten with the value of `uid`.
	 */
	int set_uid;
	unsigned int uid;

	/**
	 * If `set_mode` is non-zero, the any permissions bits set in
	 * `umask` are unset in the st_mode attribute of each file.
	 */
	int set_mode;
	unsigned int umask;

	/**
	 * The timeout in seconds for which name lookups will be
	 * cached.
	 */
	double entry_timeout;

	/**
	 * The timeout in seconds for which a negative lookup will be
	 * cached. This means, that if file did not exist (lookup
	 * retuned ENOENT), the lookup will only be redone after the
	 * timeout, and the file/directory will be assumed to not
	 * exist until then. A value of zero means that negative
	 * lookups are not cached.
	 */
	double negative_timeout;

	/**
	 * The timeout in seconds for which file/directory attributes
	 * (as returned by e.g. the `getattr` handler) are cached.
	 */
	double attr_timeout;

	/**
	 * Allow requests to be interrupted
	 */
	int intr;

	/**
	 * Specify which signal number to send to the filesystem when
	 * a request is interrupted.  The default is hardcoded to
	 * USR1.
	 */
	int intr_signal;

	/**
	 * Normally, FUSE assigns inodes to paths only for as long as
	 * the kernel is aware of them. With this option inodes are
	 * instead remembered for at least this many seconds.  This
	 * will require more memory, but may be necessary when using
	 * applications that make use of inode numbers.
	 *
	 * A number of -1 means that inodes will be remembered for the
	 * entire life-time of the file-system process.
	 */
	int remember;

	/**
	 * The default behavior is that if an open file is deleted,
	 * the file is renamed to a hidden file (.fuse_hiddenXXX), and
	 * only removed when the file is finally released.  This
	 * relieves the filesystem implementation of having to deal
	 * with this problem. This option disables the hiding
	 * behavior, and files are removed immediately in an unlink
	 * operation (or in a rename operation which overwrites an
	 * existing file).
	 *
	 * It is recommended that you not use the hard_remove
	 * option. When hard_remove is set, the following libc
	 * functions fail on unlinked files (returning errno of
	 * ENOENT): read(2), write(2), fsync(2), close(2), f*xattr(2),
	 * ftruncate(2), fstat(2), fchmod(2), fchown(2)
	 */
	int hard_remove;

	/**
	 * Honor the st_ino field in the functions getattr() and
	 * fill_dir(). This value is used to fill in the st_ino field
	 * in the stat(2), lstat(2), fstat(2) functions and the d_ino
	 * field in the readdir(2) function. The filesystem does not
	 * have to guarantee uniqueness, however some applications
	 * rely on this value being unique for the whole filesystem.
	 *
	 * Note that this does *not* affect the inode that libfuse 
	 * and the kernel use internally (also called the "nodeid").
	 */
	int use_ino;

	/**
	 * If use_ino option is not given, still try to fill in the
	 * d_ino field in readdir(2). If the name was previously
	 * looked up, and is still in the cache, the inode number
	 * found there will be used.  Otherwise it will be set to -1.
	 * If use_ino option is given, this option is ignored.
	 */
	int readdir_ino;

	/**
	 * This option disables the use of page cache (file content cache)
	 * in the kernel for this filesystem. This has several affects:
	 *
	 * 1. Each read(2) or write(2) system call will initiate one
	 *    or more read or write operations, data will not be
	 *    cached in the kernel.
	 *
	 * 2. The return value of the read() and write() system calls
	 *    will correspond to the return values of the read and
	 *    write operations. This is useful for example if the
	 *    file size is not known in advance (before reading it).
	 *
	 * Internally, enabling this option causes fuse to set the
	 * `direct_io` field of `struct fuse_file_info` - overwriting
	 * any value that was put there by the file system.
	 */
	int direct_io;

	/**
	 * This option disables flushing the cache of the file
	 * contents on every open(2).  This should only be enabled on
	 * filesystems, where the file data is never changed
	 * externally (not through the mounted FUSE filesystem).  Thus
	 * it is not suitable for network filesystems and other
	 * intermediate filesystems.
	 *
	 * NOTE: if this option is not specified (and neither
	 * direct_io) data is still cached after the open(2), so a
	 * read(2) system call will not always initiate a read
	 * operation.
	 *
	 * Internally, enabling this option causes fuse to set the
	 * `keep_cache` field of `struct fuse_file_info` - overwriting
	 * any value that was put there by the file system.
	 */
	int kernel_cache;

	/**
	 * This option is an alternative to `kernel_cache`. Instead of
	 * unconditionally keeping cached data, the cached data is
	 * invalidated on open(2) if if the modification time or the
	 * size of the file has changed since it was last opened.
	 */
	int auto_cache;

	/**
	 * The timeout in seconds for which file attributes are cached
	 * for the purpose of checking if auto_cache should flush the
	 * file data on open.
	 */
	int ac_attr_timeout_set;
	double ac_attr_timeout;

	/**
	 * If this option is given the file-system handlers for the
	 * following operations will not receive path information:
	 * read, write, flush, release, fsync, readdir, releasedir,
	 * fsyncdir, lock, ioctl and poll.
	 *
	 * For the truncate, getattr, chmod, chown and utimens
	 * operations the path will be provided only if the struct
	 * fuse_file_info argument is NULL.
	 */
	int nullpath_ok;

	/**
	 * The remaining options are used by libfuse internally and
	 * should not be touched.
	 */
	int show_help;
	char *modules;
	int debug;
};

/**
 * Connection information, passed to the ->init() method
 *
 * Some of the elements are read-write, these can be changed to
 * indicate the value requested by the filesystem.  The requested
 * value must usually be smaller than the indicated value.
 */
struct fuse_conn_info {
	/**
	 * Major version of the protocol (read-only)
	 */
	unsigned proto_major;

	/**
	 * Minor version of the protocol (read-only)
	 */
	unsigned proto_minor;

	/**
	 * Maximum size of the write buffer
	 */
	unsigned max_write;

	/**
	 * Maximum size of read requests. A value of zero indicates no
	 * limit. However, even if the filesystem does not specify a
	 * limit, the maximum size of read requests will still be
	 * limited by the kernel.
	 *
	 * NOTE: For the time being, the maximum size of read requests
	 * must be set both here *and* passed to fuse_session_new()
	 * using the ``-o max_read=<n>`` mount option. At some point
	 * in the future, specifying the mount option will no longer
	 * be necessary.
	 */
	unsigned max_read;

	/**
	 * Maximum readahead
	 */
	unsigned max_readahead;

	/**
	 * Capability flags that the kernel supports (read-only)
	 */
	unsigned capable;

	/**
	 * Capability flags that the filesystem wants to enable.
	 *
	 * libfuse attempts to initialize this field with
	 * reasonable default values before calling the init() handler.
	 */
	unsigned want;

	/**
	 * Maximum number of pending "background" requests. A
	 * background request is any type of request for which the
	 * total number is not limited by other means. As of kernel
	 * 4.8, only two types of requests fall into this category:
	 *
	 *   1. Read-ahead requests
	 *   2. Asychronous direct I/O requests
	 *
	 * Read-ahead requests are generated (if max_readahead is
	 * non-zero) by the kernel to preemptively fill its caches
	 * when it anticipates that userspace will soon read more
	 * data.
	 *
	 * Asynchronous direct I/O requests are generated if
	 * FUSE_CAP_ASYNC_DIO is enabled and userspace submits a large
	 * direct I/O request. In this case the kernel will internally
	 * split it up into multiple smaller requests and submit them
	 * to the filesystem concurrently.
	 *
	 * Note that the following requests are *not* background
	 * requests: writeback requests (limited by the kernel's
	 * flusher algorithm), regular (i.e., synchronous and
	 * buffered) userspace read/write requests (limited to one per
	 * thread), asynchronous read requests (Linux's io_submit(2)
	 * call actually blocks, so these are also limited to one per
	 * thread).
	 */
	unsigned max_background;

	/**
	 * Kernel congestion threshold parameter. If the number of pending
	 * background requests exceeds this number, the FUSE kernel module will
	 * mark the filesystem as "congested". This instructs the kernel to
	 * expect that queued requests will take some time to complete, and to
	 * adjust its algorithms accordingly (e.g. by putting a waiting thread
	 * to sleep instead of using a busy-loop).
	 */
	unsigned congestion_threshold;

	/**
	 * When FUSE_CAP_WRITEBACK_CACHE is enabled, the kernel is responsible
	 * for updating mtime and ctime when write requests are received. The
	 * updated values are passed to the filesystem with setattr() requests.
	 * However, if the filesystem does not support the full resolution of
	 * the kernel timestamps (nanoseconds), the mtime and ctime values used
	 * by kernel and filesystem will differ (and result in an apparent
	 * change of times after a cache flush).
	 *
	 * To prevent this problem, this variable can be used to inform the
	 * kernel about the timestamp granularity supported by the file-system.
	 * The value should be power of 10.  The default is 1, i.e. full
	 * nano-second resolution. Filesystems supporting only second resolution
	 * should set this to 1000000000.
	 */
	unsigned time_gran;

	/**
	 * For future use.
	 */
	unsigned reserved[22];
};
