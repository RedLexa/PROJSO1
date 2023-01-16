#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char const target_path[] = "/f1";
char const link_path1[] = "/hl";
char const link_path2[] = "/sl1";
char const link_path3[] = "/sl2";
char const link_path4[] = "/sl3";
char const link_path5[] = "/sl4";
char const link_path6[] = "/sl5";
char const link_path7[] = "/sl6";
char const link_path8[] = "/sl7";
char const link_path9[] = "/sl8";
char const link_path10[] = "/sl9";
char const link_path11[] = "/sl10";

int main() {

    // init TécnicoFS
    assert(tfs_init(NULL) != -1);

    // create file with content
    {
        int f = tfs_open(target_path, TFS_O_CREAT);
        assert(tfs_close(f) != -1);
    }

    // create hard link on a file
    assert(tfs_link(target_path, link_path1) != -1);

    // create soft link on a hard link
    assert(tfs_sym_link(link_path1, link_path2) != -1);

    // create chain - soft link on a soft link
    assert(tfs_sym_link(link_path2, link_path3) != -1);

    assert(tfs_sym_link(link_path3, link_path4) != -1);

    assert(tfs_sym_link(link_path4, link_path5) != -1);

    assert(tfs_sym_link(link_path5, link_path6) != -1);

    assert(tfs_sym_link(link_path6, link_path7) != -1);

    assert(tfs_sym_link(link_path7, link_path8) != -1);

    assert(tfs_sym_link(link_path8, link_path9) != -1);

    assert(tfs_sym_link(link_path9, link_path10) != -1);

    assert(tfs_sym_link(link_path10, link_path11) != -1);

    // Remove the soft on a hard link
    assert(tfs_unlink(link_path7) != -1);

    // Link unusable - target no longer exists
    assert(tfs_open(link_path11, TFS_O_APPEND) == -1);

    // Link usable - target exists
    assert(tfs_open(link_path5, TFS_O_APPEND) != -1);

    // create soft link on a hard link with same link name
    assert(tfs_sym_link(link_path6, link_path7) != -1);

    // Link usable - target now exists
    assert(tfs_open(link_path3, TFS_O_APPEND) != -1);

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}