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
 *  FILE: open.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Mon Apr  6 19:27:49 1998
 */

#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int
get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++) {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_VFS, "ERROR: get_empty_fd: out of file descriptors "
            "for pid %d\n", curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL        OK
 *        oflags is not valid.
 *      o EMFILE        OK
 *        The process already has the maximum number of files open.
 *      o ENOMEM        OK
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG  OK
 *        A component of filename was too long.
 *      o ENOENT        OK
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR        OK
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO         OK
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */

int
do_open(const char *filename, int oflags)
{
        int fd = -1;
        int i = 0;
        int open_files = 0;
        file_t *my_file = NULL;
        int flags  = 0;
        
        /*test only*/
        /*char buffer[1024];
        
        const char *data = buffer;
        size_t len = 0;
        
        vnode_t res_vnode;
        
        vnode_t *pe = &res_vnode;
        
        dir_namev("/dev/root/eduardo/ls", &len, &data, NULL, &pe);
        
        KASSERT(NULL != NULL);*/
        /*end of test only*/
        
        /*Get a file descriptor*/
        fd = get_empty_fd(curproc);
        
        if(fd == EMFILE){
        	dbg(DBG_PRINT, "1\n");
                return EMFILE;
        }

        
        /*Check flag is valid*/
        /*Can O_APPEND be OR'd with O_RDONLY??*/
        /*Set file modes depending on flags...*/
        switch(oflags){
                
                case O_RDONLY:
                	dbg(DBG_PRINT, "2\n");
                        flags = FMODE_READ;
                        break;
                case O_WRONLY:
                	dbg(DBG_PRINT, "3\n");
                        flags = FMODE_WRITE;
                        break;
                case O_RDWR:
                	dbg(DBG_PRINT, "4\n");
                        flags = FMODE_WRITE | FMODE_READ;
                        break;
                case O_WRONLY | O_APPEND:
                dbg(DBG_PRINT, "5\n");
                        flags = FMODE_APPEND;   /*do we need FMODE_WRITE too?*/
                        break;
                case O_RDWR | O_APPEND:
                dbg(DBG_PRINT, "6\n");
                        flags = FMODE_READ | FMODE_APPEND;      /*do we need FMODE_APPEND??*/
                        break;
                default:
                        return -EINVAL;
        }
        
        
        /*Get new file object*/
        my_file = fget(fd);     /*also increments reference count*/
        
        /*we could not allocate memory for file...*/
        if(my_file == NULL){
        	dbg(DBG_PRINT, "7\n");
                return -ENOMEM;
        }
        
        /*Set field, ref count and vnode*/
        my_file->f_mode = flags;
        my_file->f_pos = 0;
        dbg(DBG_PRINT, "8\n");
        /*get vnode, return error*/
        int res = open_namev(filename, oflags, &my_file->f_vnode, NULL);                /*CHECK: need to check if argument 3 is ok or not*/
          
        dbg(DBG_PRINT, "9\n");                                                                               
        
        
        if(/*res == ENAMETOOLONG || res == ENOENT || res == EISDIR || res == ENXIO*/ res < 0){  /*Error*/
                fput(my_file);                  /*free file memory since this is an error*/
                return res;
        }
        
        /*Assign file to process*/
        curproc->p_files[fd] = my_file;
        
        return fd;
}
