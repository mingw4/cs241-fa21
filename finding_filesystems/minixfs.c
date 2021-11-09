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
#include <sys/time.h>
#include <sys/stat.h>
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

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    inode* x = get_inode(fs, path);
    if (x) {
        if (owner != (x->uid - 1)) {
            x->uid = owner;
        }
        if (group != (x->gid - 1)) {
            x->gid = group;
        }
        clock_gettime(CLOCK_REALTIME, &x->ctim);
        return 0;
    }
    if (-1 == access(path, F_OK)) {
        errno = ENOENT;
    }
    return -1;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    if (!valid_filename(path) || get_inode(fs, path)) {
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
    if (NUM_DIRECT_BLOCKS <= k) {
        l = p->indirect;
        make_string_from_dirent(fs->data_root[*(fs->data_root[l].data+(k-NUM_DIRECT_BLOCKS)*sizeof(data_block_number))].data + (p->size%sizeof(data_block)), s);
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
        if (strlen(info) < *off) {
            return 0;
        }
        if (strlen(info) - *off < count) {
            count = strlen(info - *off);
        }
        memmove(buf, info + *off, count);
        *off = *off + count;
        return count;
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    if ((NUM_INDIRECT_BLOCKS + NUM_DIRECT_BLOCKS) * sizeof(data_block) < count + *off) {
        errno = ENOSPC;
        return -1;
    }
    inode* x = get_inode(fs, path);
    if(!x) {
        x = minixfs_create_inode_for_path(fs, path);
    }
    if (-1 == minixfs_min_blockcount(fs, path, (count + *off) / sizeof(data_block) + 1)) {
        errno = ENOSPC;
        return -1;
    }
    int j = *off / sizeof(data_block);
    int k = *off % sizeof(data_block);
    data_block_number l;
    size_t m = 0;
    while (count > m) {
        if (NUM_DIRECT_BLOCKS <= j) {
            l = *((data_block_number *) fs->data_root[x->indirect].data+(j - NUM_DIRECT_BLOCKS) * sizeof(data_block_number));
        } else {
            l = x->direct[j];
        }
        size_t n = (count - m) > (sizeof(data_block) - k) ? (sizeof(data_block) - k) : (count - m);
        memcpy(fs->data_root[l].data + k, buf + m, n);
        m = m + n;
        k = 0;
        j = j + 1;
    }
    *off = *off + count;
    x->size = *off;
    clock_gettime(CLOCK_REALTIME, &(x->mtim));
    clock_gettime(CLOCK_REALTIME, &(x->atim));

    return count;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    inode* x = get_inode(fs, path);
    if (!x) {
        errno = ENOENT;
        return -1;
    }
    int j = *off / sizeof(data_block);
    int k = *off % sizeof(data_block);
    data_block_number l;
    size_t m = 0;
    if (x->size - *off < count) {
        count = x->size - *off;
    }
    while (count > m) {
        if (NUM_DIRECT_BLOCKS <= j) {
            l = *((data_block_number *) fs->data_root[x->indirect].data+(j - NUM_DIRECT_BLOCKS) * sizeof(data_block_number));
        } else {
            l = x->direct[j];
        }
        size_t n = (count - m) > (sizeof(data_block) - k) ? (sizeof(data_block) - k) : (count - m);
        memcpy(fs->data_root[l].data + k, buf + m, n);
        m = m + n;
        k = 0;
        j = j + 1;
    }
    *off = *off + m;
    clock_gettime(CLOCK_REALTIME, &(x->atim));
    return m;


    return -1;
}
