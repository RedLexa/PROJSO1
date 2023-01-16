#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
void * buffer = "ABCDEF";
char * toread;
char * name = "/f1";
void * buffer2;
ssize_t a = 0;
ssize_t b = 0;
void *fn1(void * name2){
    char * name1 = name2;
    int f = tfs_open(name1,TFS_O_TRUNC);
    for(int i = 0; i < 3; i++){
         a += tfs_write(f,buffer + i, strlen(buffer + i));
    }
    assert(tfs_close(f) != -1);
    return NULL;
}
void *fn2(void * name2){
    char * name1 = name2;
    int f = tfs_open(name1,0);
    for(int i = 0; i < 3; i++){
        b += tfs_read(f,buffer2, strlen(buffer2));
    }
    assert(tfs_close(f) != -1);
    return NULL;
}

int main(){
    pthread_t tid[3];
    tfs_params params = tfs_default_params();
    params.max_inode_count = 3;
    params.max_block_count = 3;
    assert(tfs_init(&params) != -1);
    int f = tfs_open(name,TFS_O_CREAT);
    assert(tfs_close(f) != -1);
    if (pthread_create(&tid[0], NULL, fn1, (void *)name) != 0){
        exit(0);
    }
    if (pthread_create(&tid[1], NULL, fn2, (void *)name) != 0){
        exit(1);
    }
    pthread_join(tid[0],NULL);
    pthread_join(tid[1],NULL);

    assert(a == b);

    printf("Successful test.\n");
    return 0;
}