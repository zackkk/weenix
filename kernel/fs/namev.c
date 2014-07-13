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

#include "kernel.h"
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
* Most of the work should be done by the vnode's implementation
* specific lookup() function, but you may want to special case
* "." and/or ".." here depnding on your implementation.
*
* If dir has no lookup(), return -ENOTDIR.
*
* Note: returns with the vnode refcount on *result incremented.
*/
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        
        KASSERT(NULL != dir);
        dbg(DBG_PRINT,"(GRADING2A 2.a) dir is not NULL.\n");
        
        KASSERT(NULL != name);
        dbg(DBG_PRINT,"(GRADING2A 2.a) name is not NULL\n");
        
        KASSERT(NULL != result);
        dbg(DBG_PRINT,"(GRADING2A 2.a) result is not NULL\n");
        
        /*NOT_YET_IMPLEMENTED("VFS: lookup");*/
        int res = 0;
        
        dbg(DBG_PRINT, "Looking for file/dir: %s\n", name);
        
        /*If we dont have a lookup function, then we are not a directory...*/
        if(dir->vn_ops->lookup == NULL){
                return -ENOTDIR;
        }
        
        /*copy name to local buffer*/
        char buffer[2*NAME_LEN+1];
        memset(buffer, 0, 2*NAME_LEN+1);
        
        size_t i = 0;
        while (i < (2*NAME_LEN + 1) && i < len){
                if(*name == '\0'){
                        break;
                }
                else if(*name == '/'){
                      buffer[i] = 0;
                }
                else{
                     buffer[i] = *name;
                }
                name++;
                i++;
        }
        
        /*recalculate name length*/
        int tmplen = strlen(buffer);
        
        if(tmplen > NAME_LEN){
                return -ENAMETOOLONG;
        }

        
        
        /*look up file '/dir', use argument 'dir' to get the file system in ramfs.c lookup*/
        /* . and .. are part of the directory entry?? CHECK*/
        /*should return the vnode of name, that's in the current dir*/
        res = dir->vn_ops->lookup(dir, buffer, tmplen, result); /*this will increase result refcount */
        if(res == 0){
                
         dbg(DBG_PRINT, "Lookup succesful: Name:%s, Len:%d, node reference count: %d, \n", name, len, (*result)->vn_refcount);
                
        }
        dbg(DBG_PRINT, "look up return value: %d\n", res);

        return res;
}


/* When successful this function returns data in the following "out"-arguments:
* o res_vnode: the vnode of the parent directory of "name"
* o name: the `basename' (the element of the pathname)
* o namelen: the length of the basename
*
* For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
* &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
* vnode corresponding to "/s5fs/bin" in res_vnode.
*
* The "base" argument defines where we start resolving the path from:
* A base value of NULL means to use the process's current working directory,
* curproc->p_cwd. If pathname[0] == '/', ignore base and start with
* vfs_root_vn. dir_namev() should call lookup() to take care of resolving each
* piece of the pathname.
*
* Note: A successful call to this causes vnode refcount on *res_vnode to
* be incremented.
*/
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
        char *slash_ptr = NULL;
        char *path_ptr = NULL;
        size_t i = 0;
        char *next_slash = NULL;
        int res =0;
        int len = 0;
        int count = 0;
        
        KASSERT(NULL != pathname);
        dbg(DBG_PRINT,"(GRADING2A 2.b) pathname is not NULL.\n");
        KASSERT(NULL != namelen);
        dbg(DBG_PRINT,"(GRADING2A 2.b) namelen is not NULL.\n");
        KASSERT(NULL != name);
        dbg(DBG_PRINT,"(GRADING2A 2.b) name is not NULL.\n");
        KASSERT(NULL != res_vnode);
        dbg(DBG_PRINT,"(GRADING2A 2.b) res_vode is not NULL.\n");
        
        vnode_t *current_dir = NULL;
        vnode_t result;
        
        /*Path cannot be empty*/
        if(!strcmp(pathname, "")){
                return -EINVAL;
        }
        
        /*Buffer length... max 28 chars*/
        char current_name[NAME_LEN+1]; /*NAME_LEN is 28 + 1 for null char*/
        const char *pathname_index = pathname;
        const char *current_name_index = NULL;
        
        /*Otherwise...*/
        if(pathname[0] == '/' /*|| base == NULL*/){
                /*start from root node*/
                current_dir = vfs_root_vn;
                vref(current_dir);
        }
        else if(base == NULL){

         KASSERT(curproc->p_cwd);
         dbg(DBG_PRINT,"cur proc number: %d, not null\n", curproc->p_pid);
                current_dir = curproc->p_cwd;
                vref(current_dir);
        }
        else{
                current_dir = base;
                vref(current_dir);
        }
        
        dbg(DBG_PRINT, "Path to look-up: %s\n", pathname);
        
        /*Start looking*/
        while(1){
                
                int extra = 0;
                /*get point to char after slash*/
                if(pathname[0] != '/' && i == 0){
                        current_name_index = pathname;
                }
                else{
                        slash_ptr = strchr(pathname_index, '/');
                        current_name_index = slash_ptr + 1;
                }
                
                /*next slash*/
                next_slash = strchr(current_name_index, '/');
                
                /*double/triple slash.*/
                while(next_slash != NULL && (next_slash+1) != '\0' && *(next_slash+1) == '/'){
                        next_slash++;
                        extra++;
                }
        
                
                if(next_slash == NULL || *(next_slash + 1) == '\0'){
                        /*If we didn't find next slash, we are in the name part of the path name*/
                        KASSERT(NULL != res_vnode);
                        dbg(DBG_PRINT,"(GRADING2A 2.b) pointer to corresponding vnode is not NULL.\n");
                        *name = current_name_index; /*path name ends in null character...*/
                        *namelen = strlen(*name);

                        KASSERT(NULL != current_dir);
                        dbg(DBG_PRINT,"current_dir is not null, count:%d\n", count);
                
                        if(count == 0) {
                         *res_vnode = current_dir;
                        }
                        return 0;
                }
                else{
                        pathname_index = next_slash; /*Now we point to next slash to get next word on the following iteration*/
                }
                
                /*length of the name*/
                int len = next_slash - current_name_index - extra;
                if(len > NAME_LEN){
                        return -ENAMETOOLONG;
                }
                /*copy name in local buffer*/
                memset(current_name, 0, NAME_LEN+1); /*reset 0 chars*/
                if(len < NAME_LEN){ /*name 28 (0-27) char long, 29 item MUST be 0*/
                        memcpy(current_name, current_name_index, len); /*copy name to buffer*/
                }
                else{
                        memcpy(current_name, current_name_index, NAME_LEN);
                        current_name[NAME_LEN] = 0;
                }
                
                res = lookup(current_dir, current_name, strlen(current_name), res_vnode); /* increase res_vnode refccount */
                if(res == 0){
                 vput(current_dir);
                        dbg(DBG_PRINT, "Found directory:%s\n", current_name);
                        dbg(DBG_PRINT, "current_dir: vnode num:%d, ref count:%d\n", current_dir->vn_vno, current_dir->vn_refcount);
                        dbg(DBG_PRINT, "res_vnode: vnode num:%d, ref count:%d\n", (*res_vnode)->vn_vno, (*res_vnode)->vn_refcount);
                        current_dir = *res_vnode;
                }
                else{
                 vput(current_dir);
                        /*we did no find the current path, return error!!*/
                        /*do we need to decrement refcount on error??*/
                        dbg(DBG_PRINT, "Did not find directoy %s %d\n", current_name, res);
                        return res;
                }
                i++;
                count++;
                
        }
        
        dbg(DBG_PRINT, "Reached return out of while loop\n");
        return 0; /*WE should NOT reach this return*/
}

/* This returns in res_vnode the vnode requested by the other parameters.
* It makes use of dir_namev and lookup to find the specified vnode (if it
* exists). flag is right out of the parameters to open(2); see
* <weenix/fnctl.h>. If the O_CREAT flag is specified, and the file does
* not exist call create() in the parent directory vnode.
*
* Note: Increments vnode refcount on *res_vnode.
*/
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        /*NOT_YET_IMPLEMENTED("VFS: open_namev");*/
        int res = 0;
        size_t namelen = 0;
        const char *name = NULL;
        
        /*look for the path and file name...*/
        


        res = dir_namev(pathname, &namelen, &name, base, res_vnode);

        if(res == 0){
                /*We found the directory path, name should hold the file name*/
                /*Now use lookup to see if file exists...*/
                /*
*At this point, res_vnode should have the directory vnode
*name sohould point to the file name
**/
                
                /*TEST... Decrement parent directory vnode*/
                vput(*res_vnode);
                
                res = lookup(*res_vnode, name, strlen(name), res_vnode);
                dbg(DBG_PRINT, "open_namev res %d\n",res);
                
                
                if(res == 0){
                        
                        /*At this point res_vnode should point to the file vnode...*/
                        dbg(DBG_PRINT, "File vnode refcount %d\n", (*res_vnode)->vn_refcount);
                        return res;
                }
                else if(res == -ENOENT){
                        
                        /*If we didn't find the file, create and if flag set to O_CREATE*/
                        if(flag & O_CREAT){
                                
                               /*return the newly created vnode in res_vnode...*/
                               /*At this point res_vnode is the parent directory*/
                               /*Call the the create function in directory vnode*/
                               /*return newly create file vnode in res_vnode*/
                               res = (*res_vnode)->vn_ops->create(*res_vnode, name, strlen(name), res_vnode); /*Create file on res_vnode (which should be the directory)*/
                               dbg(DBG_PRINT, "9\n");
                               return res;
                               
                        }
                        else{
                                /*Return file not found?*/
                         dbg(DBG_PRINT, "10\n");
                                return -ENOENT; /*Not sure...*/
                        }
                        
                }
                else return res;
        }
        else{
                return res; /*We didn't find directory*/
        }
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
* to the given buffer. On success 0 is returned. If 'dir' does not
* contain 'entry' then -ENOENT is returned. If the given buffer cannot
* hold the result then it is filled with as many characters as possible
* and a null terminator, -ERANGE is returned.
*
* Files can be uniquely identified within a file system by their
* inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
* directories cannot have more than one link there is always
* a unique solution. The path is writen to the given buffer.
* On success 0 is returned. On error this function returns a
* negative error code. See the man page for getcwd(3) for
* possible errors. Even if an error code is returned the buffer
* will be filled with a valid string which has some partial
* information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
