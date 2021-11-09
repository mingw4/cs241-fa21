/**
 * finding_filesystems
 * CS 241 - Fall 2021
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include "includes/compare.h"
#include "includes/callbacks.h"
#include "includes/dictionary.h"
#include "includes/set.h"
#include "includes/vector.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>




/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
//Good
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows!
    inode* x = get_inode(fs, path);
    if (x) {
        x->mode = new_permissions | ((x->mode >> RWX_BITS_NUMBER) << RWX_BITS_NUMBER);
        clock_gettime(CLOCK_REALTIME, &x->ctim);
        return 0;
    }
    if (-1 == access(path, F_OK)) {
        errno = ENOENT;
    }
    return -1;
}

// fail when passing -1 for the uid and gid
int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    if (-1 == access(path, F_OK)) {
        errno = ENOENT;
        return -1;
    }
    inode* x = get_inode(fs, path);
    if (x != NULL) {
        if (owner != (uid_t) -1) {
            x->uid = owner;
        }
        if (group != (gid_t) -1) {
            x->gid = group;
        }
        clock_gettime(CLOCK_REALTIME, &x->ctim);
        return 0;
    }
    return -1;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    if (valid_filename(path) != 1 || get_inode(fs, path)) {
        return NULL;
    }
    const char *fn;
    minixfs_dirent s;
    data_block_number l;

    inode *p = parent_directory(fs, path, &fn);
    inode_number j = first_unused_inode(fs);
    init_inode(p, &(fs->inode_root[j]));
    s.name = strdup(fn);
    s.inode_num = j;
    int k = p->size / sizeof(data_block);
    int n = p->size % sizeof(data_block);
    if (NUM_DIRECT_BLOCKS <= k) {
        l = p->indirect;
        data_block_number m = *((data_block_number *) fs->data_root[l].data+(k - NUM_DIRECT_BLOCKS) * sizeof(data_block_number));
        make_string_from_dirent(fs->data_root[m].data + n, s);
    } else {
        l = p->direct[k];
        make_string_from_dirent(fs->data_root[l].data + (p->size % sizeof(data_block)), s);
    }
    free(s.name);
    return fs->inode_root + j;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
        char* dtmp = GET_DATA_MAP(fs->meta);
        ssize_t num = 0;
        for (uint64_t j = 0; j < fs->meta->dblock_count; j++) {
            if (1 == dtmp[j]) {
                num = num + 1;
            }
        }
        char* info = block_info_string(num);
        size_t length = strlen(info);
        if (((int)length) < *off) {
            return 0;
        }
        if (length - *off < count) {
            count = strlen(info - *off);
        }
        memmove(buf, info + *off, count);
        *off = *off + count;
        return count;
    }
    inode* x = get_inode(fs, path);
    if (x == NULL) {
        errno = ENOENT;
        return -1;
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    inode *x = get_inode(fs, path);
    if (x == NULL) {
        x = minixfs_create_inode_for_path(fs, path);
        if (x == NULL) {
            errno = ENOSPC;
            return -1;
        }
    }
    size_t i = (*off + count) / sizeof(data_block);
    if ((*off + count) % sizeof(data_block) != 0) {
        ++i;
    }
    if (minixfs_min_blockcount(fs, path, (int)i) == -1) {
        errno = ENOSPC;
        return -1;
    }
                size_t k = *off % sizeof(data_block);
    off_t l = *off + count;
    size_t j = *off / sizeof(data_block);

    size_t m = 0;
    x->size = *off + count;
    while (l > *off) {
        data_block curr_block;
        if (NUM_DIRECT_BLOCKS > j) {
            curr_block = fs->data_root[x->direct[j]];
        } else {
            data_block_number *o = (data_block_number *)(fs->data_root + x->indirect);
            curr_block = fs->data_root[o[j - NUM_DIRECT_BLOCKS]];
        }
        size_t n = sizeof(data_block) - k;
        n = ((l - *off) < (off_t)n) ? (l - *off) : n;
        memcpy(curr_block.data, buf + n, n);
        *off += n;
        m = m + n;
        k = 0;
        ++j;
    }
    clock_gettime(CLOCK_REALTIME, &x->mtim);
    clock_gettime(CLOCK_REALTIME, &x->atim);
    return m;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *i = is_virtual_path(path);
    if (i) {
        return minixfs_virtual_read(fs, i, buf, count, off);
    }
    inode *x = get_inode(fs, path);
    if (x == NULL) {
        errno = ENOENT;
        return -1;
    }
    if (x->size <= (uint64_t)*off) {
        return 0;
    }

    size_t k = *off % sizeof(data_block);
                size_t j = *off / sizeof(data_block);
    off_t l = ((*off + count) >= x->size) ? x->size : (*off + count);
    size_t p = 0;
    while (l > *off) {
        data_block curr_block;
        if (NUM_DIRECT_BLOCKS > j){
            curr_block = fs->data_root[x->direct[j]];
        } else {
            data_block_number *o = (data_block_number *)(fs->data_root + x->indirect);
            curr_block = fs->data_root[o[j - NUM_DIRECT_BLOCKS]];
        }
        size_t q = sizeof(data_block) - k;
        q = ((l - *off) < (off_t)q) ? (l - *off) : q;
        memcpy(buf + p, curr_block.data, q);
        *off += q;
        p = p + q;
        k = 0;
        ++j;
    }
    clock_gettime(CLOCK_REALTIME, &x->atim);
    return p;
}
