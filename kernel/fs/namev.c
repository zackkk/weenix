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
        /*NOT_YET_IMPLEMENTED("VFS: lookup");*/
        int res = 0;
        
        /*If directory has no lookup function...*/
        if(dir->vn_ops->lookup == NULL){
                return -ENOTDIR;
        }
        
        /*look up file/dir*/
        /* . and .. are part of the directory entry?? CHECK*/
        /*should return the vnode of name, that's in the current dir*/
        res = dir->vn_ops->lookup(dir, name, len, result);      /*this should return refcount incremented*/
                                                                 
        dbg(DBG_PRINT, "Node reference count: %d\n", (*result)->vn_refcount);

        return res;
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
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
        
        vnode_t *current_dir = NULL;
        vnode_t result;
        
        /*Buffer length... max 1024 chars*/
        char current_name[1024];
        const char *pathname_index = pathname;
        const char *current_name_index = NULL;
        
        /*special case path has no slashes... */
        if(strchr(pathname, '/') == NULL){
                *name = pathname;
                *namelen = 0;
                res_vnode = &base;
        }
        
        /*Otherwise...*/
        if(pathname[0] == '/'){
                /*start from root node*/
                current_dir = vfs_root_vn;
        }
        else if(base == NULL){
                current_dir = curproc->p_cwd;                
                
        }
        else{
                current_dir = base;
        }
        
        dbg(DBG_PRINT, "Path %s\n", pathname);
        
        /*Start looking*/
        while(1){
                
                /*get point to char after slash*/
                if(pathname[0] != '/' && i == 0){
                        current_name_index = pathname; 
                }
                else{
                        slash_ptr = strchr(pathname_index, '/');
                        current_name_index = slash_ptr + 1;
                        /*dbg(DBG_PRINT, "first slash%c\n", *(slash_ptr+ 1));*/
                }
                
                /*next slash*/
                next_slash = strchr(current_name_index, '/');
                
                
                if(next_slash == NULL){
                        /*If we didn't find next slash, we are in the name
                        part of the path name*/
                        *name = current_name_index;     /*path name ends in null character...*/
                        *namelen = i;
                        
                        dbg(DBG_PRINT, "namelen %u name %s\n", *namelen, *name);
                        
                        /*On previous iteration, we set res_vnode to the parent directory*/
                        return 0;
                        
                }
                else{
                        pathname_index = next_slash;    /*Now we point to next slash to get next word on the following iteration*/
                }
                
                /*length of the name*/
                int len = next_slash - current_name_index;
                
                dbg(DBG_PRINT, "Path section length %d\n", len);
                
                /*copy name in local buffer*/
                memset(current_name, 0, 1024);
                
                if(len < 1023){                 /*name 1023 (0-1022) char long, 1024 item MUST be 0*/
                        /*copy name to buffer*/
                        memcpy(current_name, current_name_index, len);
                }
                else{
                        memcpy(current_name, current_name_index, 1024);
                        current_name[1023] = 0;                      
                }
                
                dbg(DBG_PRINT, "Path section name: %s\n", current_name);
                
                
                /*Now get vnode...*/
                /*Use res_vnode temporarely to navigate path*/
                /*lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)*/
                res = lookup(current_dir, current_name, strlen(current_name), res_vnode);
                
                if(res == 0){
                        current_dir = *res_vnode;
                }
                else{
                        /*do we need to decrement refcount on error??*/
                        return res;    
                }
                i++;
                
        }
                
        
        return 0;
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fnctl.h>.  If the O_CREAT flag is specified, and the file does
 * not exist call create() in the parent directory vnode.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        NOT_YET_IMPLEMENTED("VFS: open_namev");
        return 0;
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
