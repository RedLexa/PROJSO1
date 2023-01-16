#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    char *path_copied_file = "/f1";
    char *path_src = "/src";

    assert(tfs_init(NULL) != -1);

    int f;


    f = tfs_open(path_src, TFS_O_CREAT);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    f = tfs_copy_from_external_fs(path_src, path_copied_file);
    assert(f == -1);


    printf("Successful test.\n");

    return 0;
}