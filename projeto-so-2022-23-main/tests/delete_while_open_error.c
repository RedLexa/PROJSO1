#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char const target_path[] = "/f1";
char const link_path[] = "/l1";


int main() {

    // init TécnicoFS
    assert(tfs_init(NULL) != -1);

    // create file with content
        int f = tfs_open(target_path, TFS_O_CREAT);


    // try delete the file while open
    assert(tfs_unlink(target_path) == -1);

    // close the file
    assert(tfs_close(f) != -1);

    //soft link on the file
    assert(tfs_sym_link(target_path, link_path) != -1);

    // delete file
    assert(tfs_unlink(target_path) != -1);

    // link unusable - targer deleted
    assert(tfs_open(link_path, TFS_O_APPEND) == -1);

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}