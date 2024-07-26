/******************************************************************************/
/* Important Spring 2022 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
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
 * specific lookup() function.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        // NOT_YET_IMPLEMENTED("VFS: lookup");
        KASSERT(NULL != dir);
        KASSERT(NULL != name);
        KASSERT(NULL != result);
        dbg(DBG_PRINT,"(GRADING2A)\n");
        if(dir->vn_ops->lookup == NULL) {
                return -ENOTDIR;
        }
        int ret_val = dir->vn_ops->lookup(dir,name,len,result);
        dbg(DBG_PRINT,"(GRADING2B)\n");
        return ret_val;
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
        // NOT_YET_IMPLEMENTED("VFS: dir_namev");
        KASSERT(NULL != pathname);
        KASSERT(NULL != namelen);
        KASSERT(NULL != name);
        KASSERT(NULL != res_vnode);
        dbg(DBG_PRINT,"(GRADING2A)\n");
        
        vnode_t *baseNode, *resultNode;//initial_baseNode;
        const char* fileName;
        int err = 0;
        int current= 0, anchor = 0, end=(int)strlen(pathname)-1;
        //Absolute path
        if (pathname[0] == '/') {
                baseNode = vfs_root_vn;
                current = 1;
                anchor = 0;
                dbg(DBG_PRINT,"(GRADING2B)\n");
        } 
        //Relative Path
        else {
                if (base == NULL) {
                        baseNode = curproc->p_cwd;
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                } else {
                        baseNode = base;
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                }
                dbg(DBG_PRINT,"(GRADING2B)\n");
                anchor = -1;
        }

        vget(baseNode->vn_fs, baseNode->vn_vno);

        if(strcmp(pathname, ".") == 0){
                *res_vnode = baseNode;
                *namelen = 1;
                *name = ".";
                //vput(baseNode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return 0;
        }
        if(strcmp(pathname, "..") == 0){
                *res_vnode = baseNode;
                *namelen = 2;
                *name = "..";
                //vput(baseNode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return 0;
        }

        //initial_baseNode = baseNode;

        // have to handle // and /./ and /./../..
        // eg: /dev/tty
        KASSERT(NULL != baseNode);
        dbg(DBG_PRINT,"(GRADING2A)\n");

        while((int)end >= 0 && pathname[end] == '/')
                end--;

        dbg(DBG_PRINT,"(GRADING2B)\n");

        while(current <= end){
                if (pathname[current] == '/'){
                        // strncpy(fileName, pathname+anchor+1, current-anchor-1);
                        // TODO: check what lookup return codes can be from the AFS
                        if(current-anchor-1 <= 0){
                                anchor = current;
                                current++;
                                dbg(DBG_PRINT,"(GRADING2B)\n");
                                continue;
                        }

                        if(current-anchor-1 > NAME_LEN){
                                vput(baseNode);
                                dbg(DBG_PRINT,"(GRADING2B)\n");
                                return -ENAMETOOLONG;
                        }

                        if((err = lookup(baseNode, pathname+anchor+1, current-anchor-1, &resultNode))){
                                vput(baseNode);
                                dbg(DBG_PRINT,"(GRADING2B)\n");
                                return err;
                        }
                        anchor = current;
                        

                        if(!S_ISDIR(resultNode->vn_mode)){
                                vput(resultNode);
                                vput(baseNode);
                                dbg(DBG_PRINT,"(GRADING2B)\n");
                                return -ENOTDIR;
                        }
                        
                               
                        vput(baseNode);
                        //if(baseNode != vfs_root_vn)
                        //        vput(baseNode);

                        baseNode = resultNode;
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        //vget(baseNode->vn_fs, baseNode->vn_vno);
                }
                current++;
        }

        dbg(DBG_PRINT,"(GRADING2B)\n");

        *res_vnode = baseNode;
        
        *namelen = current-anchor-1;

        //strncpy(*name, pathname+anchor+1, *namelen);
        *name = pathname + anchor+1;

        if(*namelen == 0)
        {
                //vput(baseNode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EEXIST;
        }

        if(*namelen > NAME_LEN){
                vput(baseNode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -ENAMETOOLONG;
        }

        /* If we have stripped "/" at the end, 
         * the file to be opened has to be a directory
         */
        if(end < (int)(strlen(pathname)-1)){
                err = lookup(baseNode, *name, *namelen, &resultNode);
                if(err < 0){
                        vput(baseNode);
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        return err;
                }
                if(!S_ISDIR(resultNode->vn_mode)){
                        vput(resultNode);
                        vput(baseNode);
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        return -ENOTDIR;
                }
                vput(resultNode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
        }
        dbg(DBG_PRINT,"(GRADING2B)\n");

        return 0;
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified and the file does
 * not exist, call create() in the parent directory vnode. However, if the
 * parent directory itself does not exist, this function should fail - in all
 * cases, no files or directories other than the one at the very end of the path
 * should be created.
 * 
 * Note: Increments vnode refcount on *res_vnode.
 */
int 
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        //NOT_YET_IMPLEMENTED("VFS: open_namev");
        vnode_t *parent_vnode, *baseNode;
        const char *name, *cur_dir = ".";
        size_t namelen;

        if (base == NULL) {
                baseNode = curproc->p_cwd;
                dbg(DBG_PRINT,"(GRADING2B)\n");
        } else {
                baseNode = base;
                dbg(DBG_PRINT,"(GRADING2B)\n");
        }
        vget(baseNode->vn_fs, baseNode->vn_vno);

        int ret = dir_namev(pathname, &namelen, &name, baseNode, &parent_vnode);
        if(ret < 0 && ret != -EEXIST ){
                vput(baseNode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return ret;
        }
        //vput(baseNode);

        if(namelen >= STR_MAX){
                vput(parent_vnode);
                vput(baseNode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -ENAMETOOLONG;
        }

        int lookup_ret;
        if(ret == -EEXIST)      
                lookup_ret = lookup(parent_vnode, cur_dir, 1, res_vnode);
        else
                lookup_ret = lookup(parent_vnode, name, namelen, res_vnode);

        if(lookup_ret == 0){
                vput(parent_vnode);
                vput(baseNode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                //vput(res_vnode);
                return 0;
        }
        else{
                if( flag&O_CREAT ){
                        KASSERT(NULL != parent_vnode->vn_ops->create);
                        dbg(DBG_PRINT,"(GRADING2A)\n");

                        int ret = (parent_vnode)->vn_ops->create(parent_vnode, name, namelen, res_vnode);
                        vput(parent_vnode);
                        vput(baseNode);
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        return ret;
                } else {
                        vput(parent_vnode);
                        vput(baseNode);
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        return lookup_ret;
                }
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




