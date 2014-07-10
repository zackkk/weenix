/******************************************************************************/
/* Important CSCI 402 usage information:                                      */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove this comment block from this file.    */
/******************************************************************************/

/*
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.5 2014/04/22 04:31:30 cvsps Exp $
 */

#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read fs_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int
do_read(int fd, void *buf, size_t nbytes)
{
        NOT_YET_IMPLEMENTED("VFS: do_read");
        return -1;
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * fs_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int
do_write(int fd, const void *buf, size_t nbytes)
{
        NOT_YET_IMPLEMENTED("VFS: do_write");
        return -1;
}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int
do_close(int fd)
{
        NOT_YET_IMPLEMENTED("VFS: do_close");
        return -1;
}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int
do_dup(int fd)
{
        NOT_YET_IMPLEMENTED("VFS: do_dup");
        return -1;
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int
do_dup2(int ofd, int nfd)
{
        NOT_YET_IMPLEMENTED("VFS: do_dup2");
        return -1;
}

/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mknod(const char *path, int mode, unsigned devid)
{
        NOT_YET_IMPLEMENTED("VFS: do_mknod");
        return -1;
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mkdir(const char *path)
{
        NOT_YET_IMPLEMENTED("VFS: do_mkdir");
        return -1;
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_rmdir(const char *path)
{
        NOT_YET_IMPLEMENTED("VFS: do_rmdir");
        return -1;
}

/*
 * Same as do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EISDIR
 *        path refers to a directory.
 *      o ENOENT
 *        A component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_unlink(const char *path)
{
	
	int res;
	
	size_t nameSize;
	vnode_t *resNode = (vnode_t*) kmalloc(sizeof(vnode_t));
	vnode_t **resNodePtr = &resNode;
	/*check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
	if((res = open_namev(path, 0, resNodePtr, NULL)) != 0) {
		dbg(DBG_PRINT, "open_namev error: %d", res);
		return res;
	}
		
	/*refcount for newly opened file*/
	dbg(DBG_PRINT, "unlink: open_namev called, refcount for %s: %d", path, (*resNodePtr)->vn_refcount);
	
	/*check that the file name does not refer to a directory*/
	if(S_ISDIR((*resNodePtr)->vn_mode)) {
		dbg(DBG_PRINT, "file name refers to directory");
		return -EISDIR;
	}
	
	size_t *namelen = (size_t*) kmalloc(sizeof(size_t));
	const char* name;
	const char **namePtr = &name; /*last element in path name stored here*/
	vnode_t *parent = (vnode_t*) kmalloc(sizeof(vnode_t));
	vnode_t **parentPtr = &parent;
	/*get parent directory - need for first arg of unlink*/
	if((res = dir_namev(path, namelen, namePtr, NULL, parentPtr)) != 0) { /*sanity check*/
		dbg(DBG_PRINT, "dir_namev error: %d", res);
		return res;
	}
	
	/*refcount for newly opened file*/
	dbg(DBG_PRINT, "unlink: dir_namev called, refcount for %s: %d", path, (*resNodePtr)->vn_refcount);
		
	/*call unlink from vnode*/
	KASSERT(NULL != parent->vn_ops->unlink);
	dbg(DBG_PRINT, "(GRADING2A 3.e) KASSERT passed - vnode->vn_ops->unlink not NULL");
	(*parentPtr)->vn_ops->unlink(((struct vnode *) (*parentPtr)), *namePtr, *namelen); 
	
	/*decrement reference counts for both the file and parent directory*/
	/*unlink does not decrement ref count, so decrement ref count of resNodePtr again, twice total*/
	vput(*resNodePtr);
	
	dbg(DBG_PRINT, "unlink: vput() called (file), refcount for %s: %d", path, (*resNodePtr)->vn_refcount);
	
	vput(*resNodePtr);
	vput(*parentPtr);
	

	dbg(DBG_PRINT, "unlink: vput() called (file), refcount for %s: %d", path, (*resNodePtr)->vn_refcount);
	dbg(DBG_PRINT, "unlink: vput() called (parDir), refcount for %s: %d", path, (*parentPtr)->vn_refcount);
	
	
	/*free alloc'ed memory*/
	kfree(resNode);
	kfree(namelen);
	kfree(parent);
	
    return 0;
}

/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 */
int
do_link(const char *from, const char *to)
{
	/* 
	 * CHECK THAT THE VNODE WE ARE LINKING FROM IS A DIR, NOT A FILE?
	 * 
	 * SHOULD WE SET OCREAT FLAG?? - IF SO, REUSE THIS CODE FOR RENAME FCN
	 */
	int res;
	
	size_t nameSize;
	vnode_t *fromNode = (vnode_t*) kmalloc(sizeof(vnode_t));
	vnode_t **fromNodePtr = &fromNode;
	/*get 'from' vnode and check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
	if((res = open_namev(from, 0, fromNodePtr, NULL)) != 0) {
		dbg(DBG_PRINT, "open_namev error: %d", res);
		return res;
	}
	
	/*refcount for newly opened file*/
	dbg(DBG_PRINT, "link: open_namev called, refcount for %s: %d", from, (*fromNodePtr)->vn_refcount);
	
	size_t *namelen = (size_t*) kmalloc(sizeof(size_t));
	const char* name;
	const char **namePtr = &name; /*last element in path name stored here*/
	vnode_t *toNode = (vnode_t*) kmalloc(sizeof(vnode_t));
	vnode_t **toNodePtr = &toNode;
	/*get 'to' vnode (directory) and check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
	if((res = dir_namev(to, namelen, namePtr, NULL, toNodePtr)) != 0) { /*sanity check*/
		dbg(DBG_PRINT, "dir_namev error: %d", res);
		return res;
	}
	
	/*refcount for newly opened file*/
	dbg(DBG_PRINT, "link: dir_namev called, refcount for %s: %d", to, (*toNodePtr)->vn_refcount);
	
	/*call link function from 'to' vnode*/
	int result = (*toNodePtr)->vn_ops->link((struct vnode*) (*fromNodePtr), (struct vnode*) (*toNodePtr), *namePtr, *namelen);
	
	/* decrement reference count for only the parent directory ('to')
	 * the ref count for the vnode linked into that directory 
	 * was already incremented by calling open_namev, and we need to keep this
	 * increased ref count now that the file is referenced by the directory
	 */
	vput(*toNodePtr);
	

	dbg(DBG_PRINT, "link: vput called (parDir), refcount for %s: %d", to, (*toNodePtr)->vn_refcount);
	
	
	/*free alloc'ed memory*/
	kfree(fromNode);
	kfree(toNode);
	kfree(namelen);
	
	return result;
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int
do_rename(const char *oldname, const char *newname)
{
    /* basically call same procedure as link, but need to specify the O_CREAT flag in open_namev, 
     * so repeat code here.
     */
	int res;
	
	size_t nameSize;
	vnode_t *oldNode = (vnode_t*) kmalloc(sizeof(vnode_t));
	vnode_t **oldNodePtr = &oldNode;
	/*get 'oldname' vnode and check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
	if((res = open_namev(oldname, O_CREAT, oldNodePtr, NULL)) != 0) {
		dbg(DBG_PRINT, "open_namev error: %d", res);
		return res;
	}
	
	/*refcount for newly opened file*/
	dbg(DBG_PRINT, "rename: open_namev called, refcount for %s: %d", oldname, (*oldNodePtr)->vn_refcount);
	
	size_t *namelen = (size_t*) kmalloc(sizeof(size_t));
	const char* name;
	const char **namePtr = &name; /*last element in path name stored here*/
	vnode_t *dirNode = (vnode_t*) kmalloc(sizeof(vnode_t));
	vnode_t **dirNodePtr = &dirNode;
	/*get 'oldname' vnode (parent directory) which is needed for link*/
	if((res = dir_namev(oldname, namelen, namePtr, NULL, dirNodePtr)) != 0) { /*sanity check*/
		dbg(DBG_PRINT, "dir_namev error: %d", res);
		return res;
	}
	/*
	 * +1 refcount for oldNode
	 * +1 refcount for dirNode
	 */
	
	/*refcount for newly opened file*/
	dbg(DBG_PRINT, "rename: dir_namev called, refcount for %s: %d", oldname, (*dirNodePtr)->vn_refcount);
	
	/*call link function from 'oldname' parent directory, specifying new name*/
	int namelength = strlen(newname);
	(*dirNodePtr)->vn_ops->link((struct vnode*) (*oldNodePtr), (struct vnode*) (*dirNodePtr), newname, namelength);
	
	/*unlink 'oldname' vnode, which is now also linked through 'newname' - just deletes one of two link to the vnode?*/
	int result = (*dirNodePtr)->vn_ops->unlink(((struct vnode *) (*dirNodePtr)), *namePtr, *namelen); 
	
	/*
	 * the two above operations (link and unlink) really just linked and unlinked the same node
	 * so no adjustment to the reference count due to these operations is necessary
	 */
	
	/*decrement reference counts for both the file and parent directory*/
	vput(*oldNodePtr);
	vput(*dirNodePtr);
	

	
	dbg(DBG_PRINT, "rename: vput called (file), refcount for %s: %d", oldname, (*oldNodePtr)->vn_refcount);
	dbg(DBG_PRINT, "rename: vput called (parDir), refcount for parent %s: %d", oldname, (*dirNodePtr)->vn_refcount);
	
	
	
	/*free alloc'ed memory*/
	kfree(oldNode);
	kfree(dirNode);
	kfree(namelen);
	
	return result;
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int
do_chdir(const char *path)
{
	int res;
	
	/*
	 * error checking in case its not a directory??
	*/
	 
	/*ERROR CHECKING*/
	vnode_t *newNode = (vnode_t*) kmalloc(sizeof(vnode_t));
	vnode_t **newNodePtr = &newNode;
	/*get 'from' vnode and check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
	if((res = open_namev(path, 0, newNodePtr, NULL)) != 0) {
		dbg(DBG_PRINT, "open_namev error: %d", res);
		return res;
	}
	/*newNodePtr should now point to the vnode of the directory given by 'path'*/
	
	/*get vnode pointed to by curproc p_cwd*/
	vnode_t *oldNode = curproc->p_cwd;
	
	/*set curproc p_cwd = new vnode. ref count for vnode should already be incremented from call to open_namev*/
	curproc->p_cwd = *newNodePtr;
	
	/*decrement ref count for previous curproc p_cwd vnode*/
	vput(oldNode);
	
	return 0;
}

/* Call the readdir fs_op on the given fd, filling in the given dirent_t*.
 * If the readdir fs_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int
do_getdent(int fd, struct dirent *dirp)
{
	/*get the file and check a valid fd was passed in*/
	struct file *currFile = fget(fd);
	if(currFile == NULL)
		return -EBADF;
	
	/*get the vnode corresponding to the file, check that it's a directory*/
	struct vnode *currNode = currFile->f_vnode;
	if(!S_ISDIR(currNode->vn_mode))
		return -ENOTDIR;
	
	/*now we have the vnode and it refers to a directory*/
	
	/*check that the virtual function exists before calling it*/
	KASSERT(NULL != currNode->vn_ops->readdir);
	
	off_t offset = 0;
	/*read the first directory entry into dirp*/
	currNode->vn_ops->readdir(currNode, offset, dirp);
	
	/*increase the directory's f_pos*/
	currFile->f_pos += offset;
	
	/*read all subsequent entries, increasing f_pos with each call*/
	while(offset != 0) {
		currNode->vn_ops->readdir(currNode, offset, dirp);
		currFile->f_pos += offset;
	}
	
	/*return the number of bytes read into the dirent*/
	return offset;
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int
do_lseek(int fd, int offset, int whence)
{
	/*get the file and check a valid fd was passed in*/
	struct file *currFile = fget(fd);
	if(currFile == NULL)
		return -EBADF;
	
	/*check whether whence == one of the three valid start points in the file*/
	/*currFile->f_vnode->*/
	
	return 0;
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_stat(const char *path, struct stat *buf)
{
	/*KASSERT(pointer to corresponding vnode ->vn_ops->stat);*/
	return -1;
}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int
do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int
do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif