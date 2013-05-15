/*****************************************************************************\
 *  Copyright (C) 2007-2010 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Brian Behlendorf <behlendorf1@llnl.gov>.
 *  UCRL-CODE-235197
 *
 *  This file is part of the SPL, Solaris Porting Layer.
 *  For details, see <http://github.com/behlendorf/spl/>.
 *
 *  The SPL is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  The SPL is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the SPL.  If not, see <http://www.gnu.org/licenses/>.
\*****************************************************************************/

#ifndef _SPL_VNODE_H
#define _SPL_VNODE_H

//#include <linux/module.h>
//#include <linux/syscalls.h>
//#include <linux/fcntl.h>
#include <sys/fcntl.h>
//#include <linux/buffer_head.h>
//#include <linux/dcache.h>
//#include <linux/namei.h>
//#include <linux/file.h>
//#include <linux/fs.h>
//#include <linux/fs_struct.h>
//#include <linux/mount.h>

#include <sys/mount.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/sunldi.h>
#include <sys/cred.h>

#include <kern/locks.h>


// Be aware that Apple defines "typedef struct vnode *vnode_t" and
// ZFS uses "typedef struct vnode vnode_t".
#undef uio_t
#undef vnode_t
#include_next <sys/vnode.h>
#define vnode_t struct vnode
#define uio_t struct uio

/*
 * Prior to linux-2.6.33 only O_DSYNC semantics were implemented and
 * they used the O_SYNC flag.  As of linux-2.6.33 the this behavior
 * was properly split in to O_SYNC and O_DSYNC respectively.
 */
#ifndef O_DSYNC
#define O_DSYNC		O_SYNC
#endif


struct caller_context;
typedef struct caller_context caller_context_t;
typedef int vcexcl_t;

enum vcexcl	{ NONEXCL, EXCL };


#if 0
#define FREAD		1
#define FWRITE		2
#define FCREAT		O_CREAT
#define FTRUNC		O_TRUNC
#define FOFFMAX		O_LARGEFILE
#define FSYNC		O_SYNC
#define FDSYNC		O_DSYNC
#define FRSYNC		O_SYNC
#define FEXCL		O_EXCL
#define FDIRECT		O_DIRECT
#define FAPPEND		O_APPEND

#define FNODSYNC	0x10000 /* fsync pseudo flag */
#define FNOFOLLOW	0x20000 /* don't follow symlinks */

#define F_FREESP	11 	/* Free file space */
#endif


#define ATTR_XVATTR	(1 << 31)
#define AT_XVATTR	ATTR_XVATTR

#define B_INVAL		0x01
#define B_TRUNC		0x02

#define   DNLC_NO_VNODE (struct vnode *)(-1)


#define IS_DEVVP(vp)    \
        (vnode_ischr(vp) || vnode_isblk(vp) || vnode_isfifo(vp))

#if 0
/*
 * The vnode AT_ flags are mapped to the Linux ATTR_* flags.
 * This allows them to be used safely with an iattr structure.
 * The AT_XVATTR flag has been added and mapped to the upper
 * bit range to avoid conflicting with the standard Linux set.
 */
#undef AT_UID
#undef AT_GID

#define AT_MODE		ATTR_MODE
#define AT_UID		ATTR_UID
#define AT_GID		ATTR_GID
#define AT_SIZE		ATTR_SIZE
#define AT_ATIME	ATTR_ATIME
#define AT_MTIME	ATTR_MTIME
#define AT_CTIME	ATTR_CTIME

#define ATTR_XVATTR	(1 << 31)
#define AT_XVATTR	ATTR_XVATTR

#define ATTR_IATTR_MASK	(ATTR_MODE | ATTR_UID | ATTR_GID | ATTR_SIZE | \
			ATTR_ATIME | ATTR_MTIME | ATTR_CTIME | ATTR_FILE)

#define CRCREAT		0x01
#define RMFILE		0x02

#define B_INVAL		0x01
#define B_TRUNC		0x02

#define LOOKUP_DIR		0x01
#define LOOKUP_XATTR		0x02
#define CREATE_XATTR_DIR	0x04
#define ATTR_NOACLCHECK		0x20

#ifdef HAVE_PATH_IN_NAMEIDATA
# define nd_dentry	path.dentry
# define nd_mnt		path.mnt
#else
# define nd_dentry	dentry
# define nd_mnt		mnt
#endif
#endif


enum rm         { RMFILE, RMDIRECTORY };        /* rm or rmdir (remove) */
enum create     { CRCREAT, CRMKNOD, CRMKDIR };  /* reason for create */

#define va_mask         va_active
#define va_nodeid   va_fileid
#define va_nblocks  va_filerev


/*
 * vnode attr translations
 */
#define AT_TYPE         VNODE_ATTR_va_type
#define AT_MODE         VNODE_ATTR_va_mode
#define AT_UID          VNODE_ATTR_va_uid
#define AT_GID          VNODE_ATTR_va_gid
#define AT_ATIME        VNODE_ATTR_va_access_time
#define AT_MTIME        VNODE_ATTR_va_modify_time
#define AT_CTIME        VNODE_ATTR_va_change_time
#define AT_SIZE         VNODE_ATTR_va_data_size


#define va_size         va_data_size
#define va_atime        va_access_time
#define va_mtime        va_modify_time
#define va_ctime        va_change_time
#define va_crtime       va_create_time
#define va_bytes        va_data_size



#if 0
typedef enum vtype {
	VNON		= 0,
	VREG		= 1,
	VDIR		= 2,
	VBLK		= 3,
	VCHR		= 4,
	VLNK		= 5,
	VFIFO		= 6,
	VDOOR		= 7,
	VPROC		= 8,
	VSOCK		= 9,
	VPORT		= 10,
	VBAD		= 11
} vtype_t;
#endif

typedef struct vnode_attr vattr;
typedef struct vnode_attr vattr_t;

/* vsa_mask values */
#define VSA_ACL                 0x0001
#define VSA_ACLCNT              0x0002
#define VSA_DFACL               0x0004
#define VSA_DFACLCNT            0x0008
#define VSA_ACE                 0x0010
#define VSA_ACECNT              0x0020
#define VSA_ACE_ALLTYPES        0x0040
#define VSA_ACE_ACLFLAGS        0x0080  /* get/set ACE ACL flags */



#if 0
typedef struct vattr {
	enum vtype	va_type;	/* vnode type */
	u_int		va_mask;	/* attribute bit-mask */
	u_short		va_mode;	/* acc mode */
	uid_t		va_uid;		/* owner uid */
	gid_t		va_gid;		/* owner gid */
	long		va_fsid;	/* fs id */
	long		va_nodeid;	/* node # */
	uint32_t	va_nlink;	/* # links */
	uint64_t	va_size;	/* file size */
	struct timespec	va_atime;	/* last acc */
	struct timespec	va_mtime;	/* last mod */
	struct timespec	va_ctime;	/* last chg */
	dev_t		va_rdev;	/* dev */
	uint64_t	va_nblocks;	/* space used */
	uint32_t	va_blksize;	/* block size */
	uint32_t	va_seq;		/* sequence */
	struct dentry	*va_dentry;	/* dentry to wire */
} vattr_t;
#endif

#if 0
struct vnode {
	struct file	*v_file;
	kmutex_t	v_lock;		/* protects vnode fields */
	uint_t		v_flag;		/* vnode flags (see below) */
	uint_t		v_count;	/* reference count */
	void		*v_data;	/* private data for fs */
	struct vfs	*v_vfsp;	/* ptr to containing VFS */
	struct stdata	*v_stream;	/* associated stream */
	enum vtype	v_type;		/* vnode type */
	dev_t		v_rdev;		/* device (VCHR, VBLK) */
	//gfp_t		v_gfp_mask;	/* original mapping gfp mask */
};
#endif

#if 0
typedef struct vn_file {
	int		f_fd;		/* linux fd for lookup */
	struct task_struct *f_task;	/* linux task this fd belongs to */
	struct file	*f_file;	/* linux file struct */
	atomic_t	f_ref;		/* ref count */
	kmutex_t	f_lock;		/* struct lock */
	loff_t		f_offset;	/* offset */
	struct vnode		*f_vnode;	/* vnode */
	struct list_head f_list;	/* list referenced file_t's */
} file_t;
#endif

extern struct vnode *vn_alloc(int flag);
//void vn_free(struct vnode *vp);
void vn_free(struct vnode *vp);

extern int vn_open(char *pnamep, enum uio_seg seg, int filemode,
                   int createmode,
                   struct vnode **vpp, enum create crwhy, mode_t umask);

extern int vn_openat(char *pnamep, enum uio_seg seg, int filemode,
                     int createmode, struct vnode **vpp, enum create crwhy,
                     mode_t umask, struct vnode *startvp);

// OSX kernel has a vn_rdwr, let's work around it.
extern int  zfs_vn_rdwr(enum uio_rw rw, struct vnode *vp, caddr_t base,
                        ssize_t len, offset_t offset, enum uio_seg seg,
                        int ioflag, rlim64_t ulimit, cred_t *cr,
                        ssize_t *residp);

#define vn_rdwr(rw, vp, base, len, off, seg, flg, limit, cr, resid)     \
    zfs_vn_rdwr((rw), (vp), (base), (len), (off), (seg), (flg), (limit), (cr), (resid))

extern int vn_remove(char *fnamep, enum uio_seg seg, enum rm dirflag);
extern int vn_rename(char *from, char *to, enum uio_seg seg);
extern int secpolicy_vnode_create_gid(const cred_t *cred);
extern int secpolicy_vnode_setid_retain(struct vnode *vp, const cred_t *cred, boolean_t issuidroot);
extern int secpolicy_vnode_remove(struct vnode *vp, const cred_t *cr);
extern int secpolicy_vnode_setids_setgids(struct vnode *vp, const cred_t *cr,
                                          gid_t gid);
extern int secpolicy_vnode_setdac(struct vnode *vp, const cred_t *cr, uid_t u);
extern int secpolicy_vnode_chown( struct vnode *vp, const cred_t *cr, uid_t u);
extern int secpolicy_xvattr(struct vnode *dvp, vattr_t *vap,
                            uid_t, const cred_t *cr, enum vtype);
extern int secpolicy_setid_clear(vattr_t *vap, struct vnode *vp,
                                 const cred_t *cr);
extern int secpolicy_basic_link(struct vnode *svp, const cred_t *cr);


#ifndef _KERNEL
extern int vn_close(struct vnode *vp, int flags, int x1, int x2, void *x3, void *x4);
extern int vn_seek(struct vnode *vp, offset_t o, offset_t *op, void *ct);

extern int vn_getattr(struct vnode *vp, vattr_t *vap, int flags, void *x3, void *x4);
extern int vn_fsync(struct vnode *vp, int flags, void *x3, void *x4);
extern int vn_space(struct vnode *vp, int cmd, struct flock *bfp, int flag,
    offset_t offset, void *x6, void *x7);
extern file_t *vn_getf(int fd);
extern void vn_releasef(int fd);
extern int vn_set_pwd(const char *filename);

int spl_vn_init_kallsyms_lookup(void);
int spl_vn_init(void);
void spl_vn_fini(void);

#define VOP_CLOSE				vn_close
#define VOP_SEEK				vn_seek
#define VOP_GETATTR				vn_getattr
#define VOP_FSYNC				vn_fsync
#define VOP_SPACE				vn_space
#define VOP_PUTPAGE(vp, o, s, f, x1, x2)	((void)0)
#define vn_is_readonly(vp)			0
#define getf					vn_getf
#define releasef				vn_releasef

#else

// KERNEL
#define VN_HOLD(vp)     vnode_getwithref(vp)

#define VN_RELE(vp)                                 \
    do {                                            \
        if ((vp) && (vp) != DNLC_NO_VNODE)          \
            vnode_put(vp);                          \
    } while (0)


#define vn_exists(vp)
#define vn_is_readonly(vp)  vnode_vfsisrdonly(vp)

extern int
VOP_CLOSE(struct vnode *vp, int flag, int count, offset_t off, void *cr, void *);
extern int
VOP_FSYNC(struct vnode *vp, int flags, void* unused, void *);
extern int
VOP_SPACE(struct vnode *vp, int cmd, void *fl, int flags, offset_t off,
          cred_t *cr, void *ctx);

extern int VOP_GETATTR(struct vnode *vp, vattr_t *vap, int flags, void *x3, void *x4);


#endif

extern struct vnode *rootdir;

static inline int
chklock(struct vnode *vp, int iomode, unsigned long long offset, ssize_t len, int fmode, void *ct)
{
    return (0);
}

/* Root directory vnode for the system a.k.a. '/' */
/* Must use vfs_rootvnode() to acquire a reference, and
 * vnode_put() to release it
 */
#if ZFS_LEOPARD_ONLY
extern struct vnode *rootvnode;
#define getrootdir()  rootvnode
#else
static inline struct vnode *
getrootdir(void)
{
        struct vnode *rvnode = vfs_rootvnode();
        if (rvnode)
                vnode_put(rvnode);
        return rvnode;
}
#endif

#ifdef ZFS_LEOPARD_ONLY
#define vn_has_cached_data(VP)  (VTOZ(VP)->z_mmapped)
#else
#define vn_has_cached_data(VP)  (VTOZ(VP)->z_mmapped || vnode_isswap(VP))
#endif

#define vn_ismntpt(vp)   (vnode_mountedhere(vp) != NULL)




#endif /* SPL_VNODE_H */
