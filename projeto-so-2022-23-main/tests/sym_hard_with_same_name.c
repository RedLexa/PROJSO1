#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char const target_path[] = "/f1";
char const link_path[] = "/f1";


int main() {

    // init TécnicoFS
    assert(tfs_init(NULL) != -1);

    // create file with content
    {
        int f = tfs_open(target_path, TFS_O_CREAT);
        assert(tfs_close(f) != -1);
    }

    // create hard link on a file with the same name
    assert(tfs_link(target_path, link_path) == -1);

    // create soft link on a hard link with the same name
    assert(tfs_sym_link(target_path, link_path) == -1);

    // destroy TécnicoFS
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}