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

/*
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.2 2018/05/27 03:57:26 cvsps Exp $
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

/*
 * Syscalls for vfs. Refer to comments or man pages for implementation.
 * Do note that you don't need to set errno, you should just return the
 * negative error code.
 */

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read vn_op
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
        // NOT_YET_IMPLEMENTED("VFS: do_read");
        if(fd < 0 || fd >= NFILES){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        file_t* file = fget(fd);
        if(file == NULL) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        if(!(file->f_mode&FMODE_READ)){
                fput(file);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        if(S_ISDIR(file->f_vnode->vn_mode) || file->f_vnode->vn_ops->read == NULL){
                fput(file);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EISDIR;
        }
        if (nbytes == 0) {
                fput(file);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return 0;
        }
        
        int bytesRead = file->f_vnode->vn_ops->read(file->f_vnode, file->f_pos,buf,nbytes);
        do_lseek(fd,bytesRead,SEEK_CUR);
        fput(file);
        dbg(DBG_PRINT,"(GRADING2B)\n");
        return bytesRead;
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * vn_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
// int
// do_write(int fd, const void *buf, size_t nbytes)
// {
//         // NOT_YET_IMPLEMENTED("VFS: do_write");
//         if(fd < 0 || fd >= NFILES)
//                 dbg(DBG_PRINT,"(GRADING2C 10)\n");
//                 return -EBADF;
        
//         file_t* f = fget(fd);
        
//         if(f == NULL) {
//                 dbg(DBG_PRINT,"(GRADING2C 11)\n");
//                 return -EBADF;
//         }
//         if((!f->f_mode)&FMODE_WRITE || (!f->f_mode)&FMODE_APPEND){
//                 fput(f);
//                 dbg(DBG_PRINT,"(GRADING2C 12)\n");
//                 return -EBADF;
//         }
//         if(f->f_mode&FMODE_APPEND) {
//                 do_lseek(fd,0,SEEK_END);
//         }
//         int bytesWritten = f->f_vnode->vn_ops->write(f->f_vnode,0,buf,nbytes);
//         f->f_pos+=bytesWritten;
//         if (bytesWritten > 0) {
//                 // KASSERT((S_ISCHR(f->f_vnode->vn_mode)) ||
//                 //         (S_ISBLK(f->f_vnode->vn_mode)) ||
//                 //         ((S_ISREG(f->f_vnode->vn_mode)) && (f->f_pos <= f->f_vnode->vn_len)));
//         }
//         fput(f);
//         return bytesWritten;
// }
int
do_write(int fd, const void *buf, size_t nbytes)
{
        // NOT_YET_IMPLEMENTED("VFS: do_write");
        if(fd < 0 || fd >= NFILES){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        file_t* file = fget(fd);
        if(file == NULL) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        if(!((file->f_mode)&FMODE_WRITE || (file->f_mode)&FMODE_APPEND)){
                fput(file);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        if(file->f_mode&FMODE_APPEND) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                do_lseek(fd,0,SEEK_END);
                dbg(DBG_PRINT,"(GRADING2C 1)\n");
        }
        int bytesWritten = file->f_vnode->vn_ops->write(file->f_vnode,file->f_pos,buf,nbytes);
        file->f_pos += bytesWritten;
        if (bytesWritten > 0) {
                KASSERT((S_ISCHR(file->f_vnode->vn_mode)) ||
                        (S_ISBLK(file->f_vnode->vn_mode)) ||
                        ((S_ISREG(file->f_vnode->vn_mode)) && (file->f_pos <= file->f_vnode->vn_len)));
                dbg(DBG_PRINT,"(GRADING2A)\n");
                dbg(DBG_PRINT,"(GRADING2B)\n");
        }
        fput(file);
        dbg(DBG_PRINT,"(GRADING2C 1)\n");
        dbg(DBG_PRINT,"(GRADING2C 2)\n");
        dbg(DBG_PRINT,"(GRADING2B)\n");
        return bytesWritten;
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
        // NOT_YET_IMPLEMENTED("VFS: do_close");
        if(fd < 0 || fd >= NFILES || curproc->p_files[fd] == NULL ) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        
        fput(curproc->p_files[fd]);
        curproc->p_files[fd] = NULL;
        
        dbg(DBG_PRINT,"(GRADING2B)\n");
        dbg(DBG_PRINT,"(GRADING2C 1)\n");
        dbg(DBG_PRINT,"(GRADING2C 2)\n");
        return 0;
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
        // NOT_YET_IMPLEMENTED("VFS: do_dup");
        if(fd < 0 || fd >= NFILES) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        file_t *file = fget(fd);
        if(file == NULL) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        int newFd = get_empty_fd(curproc);
        curproc->p_files[newFd] = file;
        dbg(DBG_PRINT,"(GRADING2B)\n");
        return newFd;
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
        // NOT_YET_IMPLEMENTED("VFS: do_dup2");
        if(ofd < 0 || ofd >= NFILES || nfd < 0 || nfd >= NFILES) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }

        if(ofd == nfd){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                if(curproc->p_files[ofd] == NULL){
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        return -EBADF;
                }
                else{
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        return nfd;
                }
        }
                

        if(curproc->p_files[nfd] != NULL){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                do_close(nfd);
        }
        file_t *file = fget(ofd);
        if(file == NULL) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }
        curproc->p_files[nfd] = file;
        dbg(DBG_PRINT,"(GRADING2B)\n");
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
        // NOT_YET_IMPLEMENTED("VFS: do_mknod");
        vnode_t *dir_vnode,*resNode;
        size_t nameLength;
        const char *name, *fileName;
        int ret;
        dbg(DBG_PRINT,"(GRADING2B)\n");
        if( (ret = dir_namev(path,&nameLength,&name,NULL, &dir_vnode)) < 0) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return ret;
        }
        if(!S_ISCHR(mode) && !S_ISBLK(mode)){
                vput(dir_vnode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EINVAL;
        }
        //strncpy(fileName, pathname+anchor+1, current-anchor-1);

        if(lookup(dir_vnode,name,nameLength,&resNode) == 0){
                vput(resNode);
                vput(dir_vnode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EEXIST;
        }
        if(dir_vnode->vn_ops->mknod == NULL || !S_ISDIR(dir_vnode->vn_mode)) {
                vput(dir_vnode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -ENOTDIR;
        }
        KASSERT(NULL != dir_vnode->vn_ops->mknod);
        dbg(DBG_PRINT,"(GRADING2A)\n");

        int returnVal = dir_vnode->vn_ops->mknod(dir_vnode,name,nameLength,mode,devid);
        vput(dir_vnode);
        return returnVal;
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
        // NOT_YET_IMPLEMENTED("VFS: do_mkdir");
        size_t nameLength;
        const char *name;
        vnode_t *dir_vnode,*resNode;
        int ret;
        if( (ret = dir_namev(path,&nameLength,&name,NULL,&dir_vnode)) < 0){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                if(ret == -EEXIST){
                        vput(dir_vnode);
                }
                return ret;
        }
        else if(lookup(dir_vnode,name,nameLength,&resNode) == 0){
                vput(resNode);
                vput(dir_vnode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                dbg(DBG_PRINT,"(GRADING2C 2)\n");
                return -EEXIST;
        }
        if(dir_vnode->vn_ops->mkdir == NULL || !S_ISDIR(dir_vnode->vn_mode)) {
                vput(dir_vnode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -ENOTDIR;
        }
        KASSERT(NULL != dir_vnode->vn_ops->mkdir);
        dbg(DBG_PRINT,"(GRADING2A)\n");
        dbg(DBG_PRINT,"(GRADING2C 2)\n");

        int returnVal = dir_vnode->vn_ops->mkdir(dir_vnode,name,nameLength);
        vput(dir_vnode);
        return returnVal;
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
        // NOT_YET_IMPLEMENTED("VFS: do_rmdir");
        vnode_t *dir_vnode,*resNode;
        size_t nameLength;
        const char* name;
        int ret;
        if( (ret = dir_namev(path,&nameLength,&name,NULL,&dir_vnode)) < 0){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return ret;
        }
        // have to check if/how to handle dir_namev error
        if (nameLength == 1 && name_match(name, ".", nameLength))
        {
                vput(dir_vnode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EINVAL;
        }

        if (nameLength == 2 && name_match(name, "..", nameLength))
        {                
                vput(dir_vnode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -ENOTEMPTY;
        }
        KASSERT(NULL != dir_vnode->vn_ops->rmdir);
        dbg(DBG_PRINT,"(GRADING2A)\n");
        dbg(DBG_PRINT,"(GRADING2B)\n");

        int returnVal = dir_vnode->vn_ops->rmdir(dir_vnode,name,nameLength);
        vput(dir_vnode);
        return returnVal;
}

/*
 * Similar to do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EPERM
 *        path refers to a directory.
 *      o ENOENT
 *        Any component in path does not exist, including the element at the
 *        very end.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_unlink(const char *path)
{
        //NOT_YET_IMPLEMENTED("VFS: do_unlink");
        vnode_t *from_vnode, *node_vnode, *dir_vnode;
        size_t namelen;
        const char *name;
        int ret;

        if( (ret = dir_namev(path, &namelen, &name, NULL, &dir_vnode)) < 0){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return ret;
        }

        if( (ret = open_namev(path, O_RDONLY, &node_vnode, NULL)) < 0){
                vput(dir_vnode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                dbg(DBG_PRINT,"(GRADING2C 2)\n");
                return ret;
        }
        // Check name length 
        // if(namelen > NAME_LEN){
        //         return -ENAMETOOLONG;
        // }

        if(S_ISDIR(node_vnode->vn_mode)){
                vput(dir_vnode);
                vput(node_vnode);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EPERM;
        }
        KASSERT(NULL != dir_vnode->vn_ops->unlink);
        dbg(DBG_PRINT,"(GRADING2A)\n");
        dbg(DBG_PRINT,"(GRADING2C 1)\n");
        dbg(DBG_PRINT,"(GRADING2C 2)\n");

        int unlink_ret = dir_vnode->vn_ops->unlink(dir_vnode, name, namelen);
        vput(dir_vnode);
        vput(node_vnode);
        return unlink_ret;
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
 *      o EPERM
 *        from is a directory.
 */
int
do_link(const char *from, const char *to)
{
        //NOT_YET_IMPLEMENTED("VFS: do_link");
        vnode_t *from_vnode, *to_parent_vnode, *vn;
        size_t namelen;
        const char *name;
        int open_ret = open_namev(from, O_RDONLY, &from_vnode, NULL);
        if(open_ret < 0){
                //If file is not present, vn would be undefined.
                //No need to free
                //vput(vn);
                dbg(DBG_PRINT,"(GRADING2D)\n");
                return open_ret;
        }
        /*if(open_ret == 0){
                vput(from_vnode);
                return -EEXIST;
        }*/
        if(S_ISDIR(from_vnode->vn_mode)){
                vput(from_vnode);
                return -EPERM;
        }
        int dir_ret = dir_namev(to, &namelen, &name, NULL , &to_parent_vnode);
        // Check name length
        // if(namelen > NAME_LEN){
        //         return -ENAMETOOLONG;
        // }
        if(dir_ret < 0){
                dbg(DBG_PRINT,"(GRADING2D)\n");
                vput(from_vnode);
                //vput(to_parent_vnode);
                return dir_ret;
        }
        int ret = lookup(to_parent_vnode, name, namelen, &vn);
        if(ret >= 0)
        {
                dbg(DBG_PRINT,"(GRADING2D)\n");
                vput(vn);
                vput(to_parent_vnode);
                vput(from_vnode);
                return -EEXIST;
        }
        int link_ret = to_parent_vnode->vn_ops->link(from_vnode, to_parent_vnode, name, namelen);
        dbg(DBG_PRINT,"(GRADING2D)\n");
        vput(from_vnode);
        vput(to_parent_vnode);
        return link_ret;

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
        //NOT_YET_IMPLEMENTED("VFS: do_rename");
        int ret;
        if( (ret = do_link(oldname, newname)) < 0){
                dbg(DBG_PRINT,"(GRADING2D)\n");
                return ret;
        }
        dbg(DBG_PRINT,"(GRADING2D)\n");
        return do_unlink(oldname);
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
        //NOT_YET_IMPLEMENTED("VFS: do_chdir");
        vnode_t *new_dir;
        int err;

        if((err = open_namev(path, O_RDONLY, &new_dir, curproc->p_cwd)) != 0){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return err;
        }

        if(!S_ISDIR(new_dir->vn_mode)){
                vput(new_dir);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -ENOTDIR;
        }

        //vget(new_dir->vn_fs, new_dir->vn_vno);
        vput(curproc->p_cwd);

        curproc->p_cwd = new_dir;
        dbg(DBG_PRINT,"(GRADING2B)\n");
        return 0;
}

/* Call the readdir vn_op on the given fd, filling in the given dirent_t*.
 * If the readdir vn_op is successful, it will return a positive value which
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
        //NOT_YET_IMPLEMENTED("VFS: do_getdent");

        vnode_t* vnode;

        if(fd < 0 || fd >= NFILES){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }

        file_t* file = fget(fd);
        
        if(file == NULL) {
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }

        if( !S_ISDIR(file->f_vnode->vn_mode) ){
                fput(file);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -ENOTDIR;
        }

        vnode = file->f_vnode;

        int offset;

        dbg(DBG_PRINT,"(GRADING2B)\n");
        dbg(DBG_PRINT,"(GRADING2C 1)\n");

        if(!vnode->vn_ops->readdir){
                fput(file);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EBADF;
        }

        if((offset = vnode->vn_ops->readdir(vnode, file->f_pos, dirp)) == 0){
                fput(file);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                dbg(DBG_PRINT,"(GRADING2C 1)\n");
                return 0;
        }

        file->f_pos += offset;

        fput(file);
        dbg(DBG_PRINT,"(GRADING2B)\n");
        dbg(DBG_PRINT,"(GRADING2C 1)\n");
        return sizeof(*dirp);
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
        // NOT_YET_IMPLEMENTED("VFS: do_lseek");
        if(fd < 0 || fd >= NFILES)
                return -EBADF;
        file_t* file = fget(fd);
        if(file == NULL) {
                return -EBADF;
        }
        if (whence == SEEK_CUR) {
                if(file->f_pos + offset < 0){
                        fput(file);
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        return -EINVAL;
                }
                file->f_pos += offset;
                dbg(DBG_PRINT,"(GRADING2B)\n");
        } else if (whence == SEEK_END)
        {
                if(file->f_vnode->vn_len+offset < 0){
                        fput(file);
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        return -EINVAL;
                }
                file->f_pos = file->f_vnode->vn_len+offset;
                dbg(DBG_PRINT,"(GRADING2B)\n");
        }
        else if(whence == SEEK_SET)
        {
                if(offset < 0){
                        fput(file);
                        dbg(DBG_PRINT,"(GRADING2B)\n");
                        return -EINVAL;
                }
                file->f_pos = offset;
                dbg(DBG_PRINT,"(GRADING2B)\n");
        }
        else{
                fput(file);
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EINVAL;
        }
        fput(file);
        dbg(DBG_PRINT,"(GRADING2B)\n");
        return file->f_pos;
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
 *      o EINVAL
 *        path is an empty string.
 */
int
do_stat(const char *path, struct stat *buf)
{
        //NOT_YET_IMPLEMENTED("VFS: do_stat");

        vnode_t *dir_vnode;
        int ret;

        if( strcmp(path, "") == 0 ){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return -EINVAL;
        }

        if( (ret = open_namev(path, O_RDONLY, &dir_vnode, curproc->p_cwd)) < 0){
                dbg(DBG_PRINT,"(GRADING2B)\n");
                return ret;
        }
        KASSERT(NULL != dir_vnode->vn_ops->stat);
        dbg(DBG_PRINT,"(GRADING2A)\n");
        dbg(DBG_PRINT,"(GRADING2B)\n");

        ret = dir_vnode->vn_ops->stat(dir_vnode, buf);
        vput(dir_vnode);

        return ret;
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



