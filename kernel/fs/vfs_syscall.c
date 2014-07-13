
/******************************************************************************/
/* Important CSCI 402 usage information: */
/* */
/* This fils is part of CSCI 402 kernel programming assignments at USC. */
/* Please understand that you are NOT permitted to distribute or publically */
/* display a copy of this file (or ANY PART of it) for any reason. */
/* If anyone (including your prospective employer) asks you to post the code, */
/* you must inform them that you do NOT have permissions to do so. */
/* You are also NOT permitted to remove this comment block from this file. */
/******************************************************************************/

/*
* FILE: vfs_syscall.c
* AUTH: mcc | jal
* DESC:
* DATE: Wed Apr 8 02:46:19 1998
* $Id: vfs_syscall.c,v 1.5 2014/04/22 04:31:30 cvsps Exp $
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
* o fget(fd)
* o call its virtual read fs_op
* o update f_pos
* o fput() it
* o return the number of bytes read, or an error
*
* Error cases you must handle for this function at the VFS level:
* o EBADF
* fd is not a valid file descriptor or is not open for reading.
* o EISDIR
* fd refers to a directory.
*
* In all cases, be sure you do not leak file refcounts by returning before
* you fput() a file that you fget()'ed.
*/
int
do_read(int fd, void *buf, size_t nbytes)
{
        dbg(DBG_PRINT, "reading file...\n");
dbg(DBG_PRINT, "do read called, fd:%d, size_t:%d\n", fd, nbytes);

        if(fd<0 || fd >= NFILES)
        {
             dbg(DBG_PRINT, "(GRADING2C) invalid fd num do_read\n");
             return -EBADF;
        }

        file_t *f = fget(fd);

        if(NULL == f)
        {
             dbg(DBG_PRINT, "(GRADING2C) fget(fd) is NULL do_read\n");
             return -EBADF;
        }
        dbg(DBG_PRINT, "(GRADING2C) file mode is : %d\n", f->f_mode);
        if(!(f->f_mode & FMODE_READ))
        {
             dbg(DBG_PRINT, "(GRADING2C) file mode is not READ do_read\n");
             fput(f);
             return -EBADF;
        }
        if (S_ISDIR(f->f_vnode->vn_mode))
        {
             dbg(DBG_PRINT, "(GRADING2C) vnode mode is not dir do_read\n");
             fput(f);
             return -EISDIR;
        }
        int returnVal = f->f_vnode->vn_ops->read(f->f_vnode, f->f_pos, buf, nbytes);

        dbg(DBG_PRINT, "returnVal:%d\n", returnVal);
        if(returnVal>0)
        {
             f -> f_pos += returnVal;
        }
        fput(f);
        return returnVal;
}

/* Very similar to do_read. Check f_mode to be sure the file is writable. If
* f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
* fs_op, and fput the file. As always, be mindful of refcount leaks.
*
* Error cases you must handle for this function at the VFS level:
* o EBADF
* fd is not a valid file descriptor or is not open for writing.
*/
int
do_write(int fd, const void *buf, size_t nbytes)
{
        dbg(DBG_PRINT, "writing file...\n");
dbg(DBG_PRINT, "do write called, fd:%d, size_t:%d\n", fd, nbytes);
        

        if(fd<0 || fd >= NFILES)
        {
             dbg(DBG_PRINT, "(GRADING2C) invalid fd num do_write\n");
             return -EBADF;
        }


        file_t *f = fget(fd);

        if(f == NULL)
        {
             dbg(DBG_PRINT, "(GRADING2C) fget(fd) is NULL do_write\n");
             return -EBADF;
        }
        dbg(DBG_PRINT, "(GRADING2C) file mode is : %d\n", f->f_mode);
        if(f->f_mode & FMODE_APPEND)
        {
             dbg(DBG_PRINT, "(GRADING2C) file mode is FMODE_APPEND do_write\n");
             do_lseek(fd, 0, SEEK_END);
             int returnVal = f->f_vnode->vn_ops->write(f->f_vnode, f->f_pos, buf, nbytes);
             f -> f_pos += returnVal;

             fput(f);
             return returnVal;
        }
        if(!(f->f_mode & FMODE_WRITE))
        { /*Need to check modes using bitwise ops...*/
             dbg(DBG_PRINT, "(GRADING2C) file mode is not FMODE_WRITE do_write\n");
             fput(f);
             return -EBADF;
        }
        int returnVal = f->f_vnode->vn_ops->write(f->f_vnode, f->f_pos, buf, nbytes);

        if(returnVal>0)
        {
             f -> f_pos += returnVal;
              
             KASSERT((S_ISCHR(f->f_vnode->vn_mode)) || (S_ISBLK(f->f_vnode->vn_mode)) || ((S_ISREG(f->f_vnode->vn_mode)) && (f->f_pos <= f->f_vnode->vn_len)));
             dbg(DBG_PRINT, "(GRADING2A 3.a) successful write kassert\n");
        }

        fput(f);
        return returnVal;
}

/*
* Zero curproc->p_files[fd], and fput() the file. Return 0 on success
*
* Error cases you must handle for this function at the VFS level:
* o EBADF
* fd isn't a valid open file descriptor.
*/
int
do_close(int fd)
{
        /*dbg(DBG_PRINT, "Process %d is trying to close fd %d\n", curproc->p_pid, fd);*/
        dbg(DBG_PRINT, "closing file...\n");
        
        if(fd<0 || fd >= NFILES)
        {
             dbg(DBG_PRINT, "(GRADING2C) invalid fd num do_close\n");
             return -EBADF;
        }
        file_t *f;
        if((f = fget(fd)) == NULL)
        {
             dbg(DBG_PRINT, "(GRADING2C) fget(fd) is NULL do_close\n");
             return -EBADF;
        }
        /* vput(f->f_vnode);*/
         dbg(DBG_PRINT, "Process %d closed file with vnode %d and refcount %d\n", curproc->p_pid, curproc->p_files[fd]->f_vnode->vn_vno, curproc->p_files[fd]->f_vnode->vn_refcount);

        curproc->p_files[fd] = 0;
        fput(f); /*fput does vput*/
        fput(f);
        return 0;
}

/* To dup a file:
* o fget(fd) to up fd's refcount
* o get_empty_fd()
* o point the new fd to the same file_t* as the given fd
* o return the new file descriptor
*
* Don't fput() the fd unless something goes wrong. Since we are creating
* another reference to the file_t*, we want to up the refcount.
*
* Error cases you must handle for this function at the VFS level:
* o EBADF
* fd isn't an open file descriptor.
* o EMFILE
* The process already has the maximum number of file descriptors open
* and tried to open a new one.
*/
int
do_dup(int fd)
{
        dbg(DBG_PRINT, "duping file...\n");
        if(fd<0 || fd >= NFILES)
        {
             dbg(DBG_PRINT, "(GRADING2C) invalid fd num do_dup\n");
             return -EBADF;
        }
        file_t *f;
        if((f = fget(fd)) == NULL)
        {
             dbg(DBG_PRINT, "(GRADING2C) fget(fd) is NULL do_dup\n");
             return -EBADF;
        }
        
        int dfd = get_empty_fd(curproc);
        if(dfd == -EMFILE)
        {
             dbg(DBG_PRINT, "(GRADING2C) maximum number of fd open do_dup\n");
             fput(f);
             return -EMFILE;
        }
        
        dbg(DBG_PRINT, "Duplicate file vnode %d, refcount %d\n", f->f_vnode->vn_vno, f->f_vnode->vn_refcount);

        curproc->p_files[dfd]=f;
        
        
        
        return dfd;
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
* they give it to us in 'nfd'. If nfd is in use (and not the same as ofd)
* do_close() it first. Then return the new file descriptor.
*
* Error cases you must handle for this function at the VFS level:
* o EBADF
* ofd isn't an open file descriptor, or nfd is out of the allowed
* range for file descriptors.
*/
int
do_dup2(int ofd, int nfd)
{
        dbg(DBG_PRINT, "dup2ing file...\n");
        if(ofd<0 || ofd >= NFILES)
        {
             dbg(DBG_PRINT, "(GRADING2C) invalid ofd num do_dup2\n");
             return -EBADF;
        }
        if(nfd<0 || nfd >= NFILES)
        {
             dbg(DBG_PRINT, "(GRADING2C) invalid nfd num do_dup2\n");
             return -EBADF;
        }
        file_t *f;
        if((f = fget(ofd)) == NULL)
        {
             dbg(DBG_PRINT, "(GRADING2C) fget(fd) is NULL do_dup2\n");
             return -EBADF;
        }
        dbg(DBG_PRINT, "Duplicate file vnode %d, refcount %d\n", f->f_vnode->vn_vno, f->f_vnode->vn_refcount);
        if(ofd != nfd)
        {
             dbg(DBG_PRINT, "(GRADING2C) give f=fget(ofd) to nfd do_dup2\n");
             if(curproc->p_files[nfd] !=NULL)
             {
                  dbg(DBG_PRINT, "(GRADING2C) close nfd which is in use do_dup2\n");
                  do_close(nfd);
             }
             curproc->p_files[nfd]=f;
             fref(f);
        }
        
        fput(f);
        
        dbg(DBG_PRINT, "Duplicate file vnode %d, refcount %d\n", f->f_vnode->vn_vno, f->f_vnode->vn_refcount);
        
        return nfd;
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
* o EINVAL
* mode requested creation of something other than a device special
* file.
* o EEXIST
* path already exists.
* o ENOENT
* A directory component in path does not exist.
* o ENOTDIR
* A component used as a directory in path is not, in fact, a directory.
* o ENAMETOOLONG
* A component of path was too long.
*/
int
do_mknod(const char *path, int mode, unsigned devid)
{
        
        dbg(DBG_PRINT, "mknod file...\n");
        
        if(!(S_ISCHR (mode) || S_ISBLK(mode)))
        {
              dbg(DBG_PRINT, "(GRADING2C) mode is not S_IFCHR or S_IFBLK do_mknod\n");
              return -EINVAL;
        }
        size_t namelen = 0;
        const char *name;
        vnode_t *res_vnode;
        if(dir_namev(path, &namelen, &name, NULL, &res_vnode)!=0)
        {
             dbg(DBG_PRINT, "(GRADING2C) A directory component in path does not exist. dir_name fail do_mknod\n");
             return -ENOENT;
        }
        /*if(res_vnode ==NULL)
{
return -ENOENT;
}*/
        if (namelen >= NAME_LEN)
        {
             dbg(DBG_PRINT, "(GRADING2C) name is too long do_mknod\n");
             vput(res_vnode);
             return -ENAMETOOLONG;
        }
        if(!S_ISDIR(res_vnode->vn_mode))
        {
             dbg(DBG_PRINT, "(GRADING2C) vnode mode is not S_IFDIR do_mknod\n");
             vput(res_vnode);
             return -ENOTDIR;
        }

        vnode_t *result;
        if(lookup(res_vnode, name, namelen, &result) == 0)
        {
              dbg(DBG_PRINT, "(GRADING2C) path already exists do_mknod\n");
              vput(res_vnode);
              vput(result);
              return -EEXIST;
        }
        KASSERT(NULL != res_vnode->vn_ops->mknod);
        dbg(DBG_PRINT, "(GRADING2A 3.b) mknod operation is not NULL\n");
        dbg(DBG_PRINT, "refcount = %d, respages = %d\n", res_vnode->vn_refcount, res_vnode->vn_nrespages);
        vput(res_vnode);
        return res_vnode->vn_ops->mknod(res_vnode,name,namelen,mode,devid);
        
}

/* Use dir_namev() to find the vnode of the dir we want to make the new
* directory in. Then use lookup() to make sure it doesn't already exist.
* Finally call the dir's mkdir vn_ops. Return what it returns.
*
* Error cases you must handle for this function at the VFS level:
* o EEXIST
* path already exists.
* o ENOENT
* A directory component in path does not exist.
* o ENOTDIR
* A component used as a directory in path is not, in fact, a directory.
* o ENAMETOOLONG
* A component of path was too long.
*/
int
do_mkdir(const char *path)
{
        dbg(DBG_PRINT, "mkdir file...\n");
dbg(DBG_PRINT, "path %s\n", path);

        size_t namelen = 0;
        const char *name = NULL;
        vnode_t *res_vnode = NULL;


        if(dir_namev(path, &namelen, &name, NULL, &res_vnode)!= 0)
        {
             dbg(DBG_PRINT, "(GRADING2C) A directory component in path does not exist. dir_name fail do_mkdir\n");
             return -ENOENT;
        }
        
        
        dbg(DBG_PRINT,"res_node:%p\n", res_vnode);
        dbg(DBG_PRINT,"res_node address:%p\n", &res_vnode);
        dbg(DBG_PRINT, "2\n");
        

        if (namelen >= NAME_LEN)
        {
             dbg(DBG_PRINT, "(GRADING2C) name is too long do_mkdir\n");
             vput(res_vnode);
             return -ENAMETOOLONG;
        }
        dbg(DBG_PRINT, "3\n");

        KASSERT(res_vnode);
        dbg(DBG_PRINT,"res_vnode is not null\n");

        if(!S_ISDIR(res_vnode->vn_mode))
        {
             dbg(DBG_PRINT, "(GRADING2C) vnode mode is not S_IFDIR do_mkdir\n");
             vput(res_vnode);
             return -ENOTDIR;
        }
        dbg(DBG_PRINT, "4\n");
        vnode_t *result = NULL;
        if(!lookup(res_vnode, name, namelen, &result))
        {
              dbg(DBG_PRINT, "(GRADING2C) path already exists do_mkdir\n");
              vput(res_vnode);
              vput(result);
              return -EEXIST;
        }
        KASSERT(NULL != res_vnode->vn_ops->mkdir);
        dbg(DBG_PRINT, "(GRADING2A 3.c) mkdir operation is not NULL\n");
        
        
        int res = res_vnode->vn_ops->mkdir(res_vnode,name,namelen); /* hasn't increased refcoun*/
        vput(res_vnode);
        return res;
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
* removed. Then call the containing dir's rmdir v_op. The rmdir v_op will
* return an error if the dir to be removed does not exist or is not empty, so
* you don't need to worry about that here. Return the value of the v_op,
* or an error.
*
* Error cases you must handle for this function at the VFS level:
* o EINVAL
* path has "." as its final component.
* o ENOTEMPTY
* path has ".." as its final component.
* o ENOENT
* A directory component in path does not exist.
* o ENOTDIR
* A component used as a directory in path is not, in fact, a directory.
* o ENAMETOOLONG
* A component of path was too long.
*/
int
do_rmdir(const char *path)
{
        dbg(DBG_PRINT, "removing file...\n");
        size_t namelen = 0;
        const char *name;
        vnode_t *res_vnode;
        if(dir_namev(path, &namelen, &name, NULL, &res_vnode)!=0)
        {
             dbg(DBG_PRINT, "(GRADING2C) A directory component in path does not exist. dir_name fail do_rmdir\n");
             return -ENOENT;
        }

        if (namelen >= NAME_LEN)
        {
             dbg(DBG_PRINT, "(GRADING2C) name is too long do_rmdir\n");
             vput(res_vnode);
             return -ENAMETOOLONG;
        }
        if(!S_ISDIR(res_vnode->vn_mode))
        {
             dbg(DBG_PRINT, "(GRADING2C) vnode mode is not S_IFDIR do_rmdir\n");
             vput(res_vnode);
             return -ENOTDIR;
        }
        if(name_match(".", name, namelen))
        {
             dbg(DBG_PRINT, "(GRADING2C) path has \".\" as its final component do_rmdir\n");
             vput(res_vnode);
             return -EINVAL;
        }
        if(name_match("..", name, namelen))
        {
             dbg(DBG_PRINT, "(GRADING2C) path has \"..\" as its final component do_rmdir\n");
             vput(res_vnode);
             return -ENOTEMPTY;
        }
        KASSERT(NULL != res_vnode->vn_ops->rmdir);
        dbg(DBG_PRINT, "(GRADING2A 3.d) rmdir operation is not NULL\n");
        int returnval = res_vnode->vn_ops->rmdir(res_vnode,name,namelen);
        vput(res_vnode);
        return returnval;
}

/*
* Same as do_rmdir, but for files.
*
* Error cases you must handle for this function at the VFS level:
* o EISDIR
* path refers to a directory.
* o ENOENT
* A component in path does not exist.
* o ENOTDIR
* A component used as a directory in path is not, in fact, a directory.
* o ENAMETOOLONG
* A component of path was too long.
*/
int
do_unlink(const char *path)
{

        dbg(DBG_PRINT, "unlinking file...\n");
        int res;
        
        size_t nameSize;
        vnode_t *resNodePtr = NULL;
        /*check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
        if((res = open_namev(path, 0, &resNodePtr, NULL)) != 0) {
                dbg(DBG_PRINT, "open_namev error: %d\n", res);
                return res;
        }
        
        /*refcount for newly opened file*/
        
        /*check that the file name does not refer to a directory*/
        if(S_ISDIR(resNodePtr->vn_mode)) {
                dbg(DBG_PRINT, "file name refers to directory\n");
                vput(resNodePtr);
                return -EISDIR;
        }
        
        size_t *namelen = (size_t*) kmalloc(sizeof(size_t));
        const char *namePtr = NULL; /*last element in path name stored here*/
        vnode_t *parentPtr = NULL;
        
        /*get parent directory - need for first arg of unlink*/
        if((res = dir_namev(path, namelen, &namePtr, NULL, &parentPtr)) != 0) { /*sanity check*/
                dbg(DBG_PRINT, "dir_namev error: %d\n", res);
                return res;
        }
        
        /*refcount for newly opened file*/
        
        /*call unlink from vnode*/
        KASSERT(NULL != parentPtr->vn_ops->unlink);
        dbg(DBG_PRINT, "(GRADING2A 3.e) KASSERT passed - vnode->vn_ops->unlink not NULL\n");
        parentPtr->vn_ops->unlink(((struct vnode *) parentPtr), namePtr, *namelen);
        
        /*decrement reference counts for both the file and parent directory*/
        /*unlink does not decrement ref count, so decrement ref count of resNodePtr again, twice total*/
        vput(resNodePtr);
        vput(parentPtr);
        
        dbg(DBG_PRINT, "resnode vnode %d refcount %d, parent vnode %d refcount %d", resNodePtr->vn_vno, resNodePtr->vn_refcount, parentPtr->vn_vno, parentPtr->vn_refcount);
        
        
        return 0;
}

/* To link:
* o open_namev(from)
* o dir_namev(to)
* o call the destination dir's (to) link vn_ops.
* o return the result of link, or an error
*
* Remember to vput the vnodes returned from open_namev and dir_namev.
*
* Error cases you must handle for this function at the VFS level:
* o EEXIST
* to already exists.
* o ENOENT
* A directory component in from or to does not exist.
* o ENOTDIR
* A component used as a directory in from or to is not, in fact, a
* directory.
* o ENAMETOOLONG
* A component of from or to was too long.
*/
int
do_link(const char *from, const char *to)
{
        dbg(DBG_PRINT, "linking file...\n");
        /*
        * CHECK THAT THE VNODE WE ARE LINKING FROM IS A DIR, NOT A FILE?
        *
        * SHOULD WE SET OCREAT FLAG?? - IF SO, REUSE THIS CODE FOR RENAME FCN
        */
        int res;
        
        size_t nameSize;
        vnode_t *fromNodePtr = NULL;
        /*get 'from' vnode and check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
        if((res = open_namev(from, 0, &fromNodePtr, NULL)) != 0) {
                dbg(DBG_PRINT, "open_namev error: %d", res);
                return res;
        }
        
        /*refcount for newly opened file*/
        dbg(DBG_PRINT, "link: open_namev called, refcount for %s: %d", from, fromNodePtr->vn_refcount);
        
        size_t *namelen = (size_t*) kmalloc(sizeof(size_t));
        const char *namePtr = NULL; /*last element in path name stored here*/
        vnode_t *toNodePtr = NULL;
        /*get 'to' vnode (directory) and check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
        if((res = dir_namev(to, namelen, &namePtr, NULL, &toNodePtr)) != 0) { /*sanity check*/
                dbg(DBG_PRINT, "dir_namev error: %d", res);
                vput(fromNodePtr);
                return res;
        }

        /*refcount for newly opened file*/
        dbg(DBG_PRINT, "link: dir_namev called, refcount for %s: %d", to, toNodePtr->vn_refcount);
        
        /*call link function from 'to' vnode*/
        int result = toNodePtr->vn_ops->link((struct vnode*) fromNodePtr, (struct vnode*) toNodePtr, namePtr, *namelen);
        
        /* decrement reference count for only the parent directory ('to')
        * the ref count for the vnode linked into that directory
        * was already incremented by calling open_namev, and we need to keep this
        * increased ref count now that the file is referenced by the directory
        */
        vput(fromNodePtr);
        vput(toNodePtr);
        
        return result;
}

/* o link newname to oldname
* o unlink oldname
* o return the value of unlink, or an error
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
        dbg(DBG_PRINT, "renaming file...\n");
        int res;
        
        size_t nameSize;
        vnode_t *oldNodePtr = NULL;
        /*get 'oldname' vnode and check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
        if((res = open_namev(oldname, O_CREAT, &oldNodePtr, NULL)) != 0) {
        dbg(DBG_PRINT, "open_namev error: %d", res);
        return res;
        }
        
        /*refcount for newly opened file*/
        dbg(DBG_PRINT, "rename: open_namev called, refcount for %s: %d", oldname, oldNodePtr->vn_refcount);
        
        size_t *namelen = (size_t*) kmalloc(sizeof(size_t));
        const char *namePtr = NULL; /*last element in path name stored here*/
        vnode_t *dirNodePtr = NULL;
        /*get 'oldname' vnode (parent directory) which is needed for link*/
        if((res = dir_namev(oldname, namelen, &namePtr, NULL, &dirNodePtr)) != 0) { /*sanity check*/
        dbg(DBG_PRINT, "dir_namev error: %d", res);
        vput(oldNodePtr);
        return res;
        }
        /*
        * +1 refcount for oldNode
        * +1 refcount for dirNode
        */
        
        /*refcount for newly opened file*/
        dbg(DBG_PRINT, "rename: dir_namev called, refcount for %s: %d", oldname, dirNodePtr->vn_refcount);
        
        /*call link function from 'oldname' parent directory, specifying new name*/
        int namelength = strlen(newname);
        dirNodePtr->vn_ops->link((struct vnode*) oldNodePtr, (struct vnode*) dirNodePtr, newname, namelength);
        
        /*unlink 'oldname' vnode, which is now also linked through 'newname' - just deletes one of two link to the vnode?*/
        int result = dirNodePtr->vn_ops->unlink(((struct vnode *) dirNodePtr), namePtr, *namelen);
        
        /*
        * the two above operations (link and unlink) really just linked and unlinked the same node
        * so no adjustment to the reference count due to these operations is necessary
        */
        
        /*decrement reference counts for both the file and parent directory*/
        vput(oldNodePtr);
        vput(dirNodePtr);
        
        
        
        dbg(DBG_PRINT, "rename: vput called (file), refcount for %s: %d", oldname, oldNodePtr->vn_refcount);
        dbg(DBG_PRINT, "rename: vput called (parDir), refcount for parent %s: %d", oldname, dirNodePtr->vn_refcount);
        
        return result;
}

/* Make the named directory the current process's cwd (current working
* directory). Don't forget to down the refcount to the old cwd (vput()) and
* up the refcount to the new cwd (open_namev() or vget()). Return 0 on
* success.
*
* Error cases you must handle for this function at the VFS level:
* o ENOENT
* path does not exist.
* o ENAMETOOLONG
* A component of path was too long.
* o ENOTDIR
* A component of path is not a directory.
*/
int
do_chdir(const char *path)
{
int res;

        dbg(DBG_PRINT, "chdiring...\n");
        
        dbg(DBG_PRINT, "Path %s\n", path);
        /*
        * error checking in case its not a directory??
        */
        dbg(DBG_PRINT, "Curproc %d current directory %p\n", curproc->p_pid, curproc->p_cwd);
        /*ERROR CHECKING*/
        vnode_t *newNodePtr = NULL;
        /*get 'from' vnode and check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
        if((res = open_namev(path, 0, &newNodePtr, NULL)) != 0) {
        dbg(DBG_PRINT, "open_namev error: %d\n", res);
        return res;
        }
                
        if(!S_ISDIR(newNodePtr->vn_mode)){
                vput(newNodePtr);
                return -ENOTDIR;
        }

                
        /*newNodePtr should now point to the vnode of the directory given by 'path'*/
        
        /*get vnode pointed to by curproc p_cwd*/
        vnode_t *oldNode = curproc->p_cwd;
        
        /*set curproc p_cwd = new vnode. ref count for vnode should already be incremented from call to open_namev*/
        curproc->p_cwd = newNodePtr;
                
                dbg(DBG_PRINT, "Curproc %d current directory %p\n", curproc->p_pid, curproc->p_cwd);
        
        /*decrement ref count for previous curproc p_cwd vnode*/
        vput(oldNode);
                
                dbg(DBG_PRINT, "Changed to %s directory\n", path);
        
        return 0;
}

/* Call the readdir fs_op on the given fd, filling in the given dirent_t*.
* If the readdir fs_op is successful, it will return a positive value which
* is the number of bytes copied to the dirent_t. You need to increment the
* file_t's f_pos by this amount. As always, be aware of refcounts, check
* the return value of the fget and the virtual function, and be sure the
* virtual function exists (is not null) before calling it.
*
* Return either 0 or sizeof(dirent_t), or -errno.
*
* Error cases you must handle for this function at the VFS level:
* o EBADF
* Invalid file descriptor fd.
* o ENOTDIR
* File descriptor does not refer to a directory.
*/
int
do_getdent(int fd, struct dirent *dirp)
{
        /* NOT_YET_IMPLEMENTED("VFS: do_getdent"); */
        /*
* check ramfs.c: int ramfs_mount(struct fs *fs) for tutorial purpose
* file.h: struct file *fget(int fd);
*/
        
        dbg(DBG_PRINT, "getdenting...\n");
        if(fd<0 || fd >= NFILES)
        {
             dbg(DBG_PRINT, "(GRADING2C) invalid fd num do_read\n");
             return -EBADF;
        }
        file_t *tmp_file = fget(fd);
        int ret_readdir = 0;

        /* fd is not an open file descriptor. */
        if(NULL == tmp_file){
                return -EBADF;
        }

        /* File descriptor does not refer to a directory. */
        if(!S_ISDIR(tmp_file->f_vnode->vn_mode)){
                fput(tmp_file);
                return -ENOTDIR;
        }
        
        dbg(DBG_PRINT, "dir->f_pos is %d\n", tmp_file->f_pos);

        /* If the end of the file as been reached (offset == file->vn_len),
no directory entry will be read and 0 will be returned. */
      
        /* vnode.h: int (*readdir)(struct vnode *dir, off_t offset, struct dirent *d); */
        ret_readdir = tmp_file->f_vnode->vn_ops->readdir(tmp_file->f_vnode, tmp_file->f_pos, dirp);
        if(ret_readdir > 0){
                dbg(DBG_PRINT, "ret_readdir offset %d, size of directory entry struct %d\n", ret_readdir, sizeof(dirent_t));
                tmp_file->f_pos += ret_readdir;
                dbg(DBG_PRINT, "2\n");
                fput(tmp_file);
                return sizeof(dirent_t);
        }
        else{
                dbg(DBG_PRINT, "3\n");
                fput(tmp_file);
                return 0;
        }

}

/*
* Modify f_pos according to offset and whence.
*
* Error cases you must handle for this function at the VFS level:
* o EBADF
* fd is not an open file descriptor.
* o EINVAL
* whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
* file offset would be negative.
*/
int
do_lseek(int fd, int offset, int whence)
{
        /* NOT_YET_IMPLEMENTED("VFS: do_lseek"); */
        /*
* file.h struct file *fget(int fd);
*/
        dbg(DBG_PRINT, "lseeking file...\n");
        
        
        if(fd<0 || fd >= NFILES)
        {
             dbg(DBG_PRINT, "(GRADING2C) invalid fd num do_read\n");
             return -EBADF;
        }
        file_t *tmp_file = fget(fd);

        /* fd is not an open file descriptor. */
        if(NULL == tmp_file){
                return -EBADF;
        }
        /* whence is not one of SEEK_SET, SEEK_CUR, SEEK_END */
        int previous_fpos = tmp_file->f_pos;
        dbg(DBG_PRINT, "fd is %d, fpos is %d\n", fd, previous_fpos);
        switch(whence){
                case SEEK_SET:
                        tmp_file->f_pos = offset;
                        break;
                case SEEK_CUR:
                        tmp_file->f_pos += offset;
                        break;
                case SEEK_END:
                        tmp_file->f_pos = tmp_file->f_vnode->vn_len + offset;
                        break;
                default:
                        fput(tmp_file); /*free file memory*/
                        return -EINVAL;
        }
        
        dbg(DBG_PRINT,"Current vn_len %d\n", tmp_file->f_vnode->vn_len);
        
        
        /* the resulting file offset would be negative. */
        if(tmp_file->f_pos < 0){
                tmp_file->f_pos = previous_fpos;
                fput(tmp_file); /*free file memory*/
                return -EINVAL;
        }
        int retval = tmp_file->f_pos;
        fput(tmp_file); /*free file memory*/
                                 
        dbg(DBG_PRINT,"returning position... %d\n", retval);
        return retval; /*return position*/
}

/*
* Find the vnode associated with the path, and call the stat() vnode operation.
*
* Error cases you must handle for this function at the VFS level:
* o ENOENT
* A component of path does not exist.
* o ENOTDIR
* A component of the path prefix of path is not a directory.
* o ENAMETOOLONG
* A component of path was too long.
*/
int
do_stat(const char *path, struct stat *buf)
{
        /* NOT_YET_IMPLEMENTED("VFS: do_stat");
        * modified by Zack
        */
        
        dbg(DBG_PRINT, "stating file...\n");
        int res_open_namev = 0;
        int res_stat = 0;
        int res = 0;
        
        dbg(DBG_PRINT, "Calling do_stat with path %s\n", path);
        /*
        * namev.c: int open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
        * fchtl.h: #define O_CREAT 0x100 Create file if non-existent.
        */
        
        /*ERROR CHECKING*/
        vnode_t *newNodePtr = NULL;
        /*get 'from' vnode and check path (ENAMETOOLONG, ENOENT, and ENOTDIR errors)*/
        if((res = open_namev(path, 0, &newNodePtr, NULL)) != 0) {
                dbg(DBG_PRINT, "open_namev error: %d\n", res);
                return res;
        }
        /*newNodePtr should now point to the vnode of the directory given by 'path'*/
                
        /*
        * vnode.h: int (*stat)(struct vnode *vnode, struct stat *buf);
        */
        KASSERT(newNodePtr->vn_ops->stat);
        dbg(DBG_PRINT,"(GRADING2A 3.f) /pointer to corresponding vnode/->vn_ops->stat is not NULL\n");
        res_stat = newNodePtr->vn_ops->stat(newNodePtr, buf);
        
        dbg(DBG_PRINT, "res_stat return value %d, buf stat inode %d, path %s\n", res_stat, buf->st_ino, path);
        dbg(DBG_PRINT, "Return value %d\n", res_stat);
        vput(newNodePtr);
        
        return res_stat;

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
