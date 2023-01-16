#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>



char * name = "/f1";
char * link_path = "/l1";

void* fn1(void* name2){
    char* name1 = name2;
    assert(tfs_sym_link(name1,link_path) != -1);
    return NULL;
}

void* fn2(void *name2){
    char *name1 = name2;
    assert(tfs_unlink(name1) != -1);
    return NULL;
}


int main(){
    pthread_t tid[2];
    tfs_params params = tfs_default_params();
    params.max_inode_count = 2;
    params.max_block_count = 2;
    assert(tfs_init(&params) != -1);
    {
        int f = tfs_open(name, TFS_O_CREAT);
        assert(tfs_close(f) != -1);
    }

    if (pthread_create(&tid[0], NULL, fn1, (void*)name)){
        exit(0);
    }
    if (pthread_create(&tid[1], NULL, fn2, (void*)name)){
        exit(0);
    }

    pthread_join(tid[0],NULL);
    pthread_join(tid[1],NULL);

    assert(tfs_open(link_path,TFS_O_APPEND) == -1);


    printf("Successful test.\n");
    return 0;
}