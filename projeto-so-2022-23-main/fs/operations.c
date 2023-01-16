#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "betterassert.h"
#define BUFFER_LEN 128

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    if(inode_get(ROOT_DIR_INUM) != root_inode){
        return -1;
    }
    if(!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;
    return find_in_dir(root_inode, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    char buffer[MAX_FILE_NAME];
    if (!valid_pathname(name)) {
        return -1;
    }
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    int inum = tfs_lookup(name, root_dir_inode);
    size_t offset;
    if (inum >= 0) {
        // The file already exists
        inode_t  * inode = inode_get(inum);
       // pthread_rwlock_rdlock(&inode->trinco);
        if(inode->i_node_type == T_SYMLINK){
            strncpy(buffer, inode->sympath, MAX_FILE_NAME);
           // pthread_rwlock_unlock(&inode->trinco);
            return tfs_open(buffer,mode);
        }
        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");

        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
               // pthread_rwlock_wrlock(&inode->trinco);
                inode->i_size = 0;
               // pthread_rwlock_unlock(&inode->trinco);
            }
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
           // pthread_rwlock_rdlock(&inode->trinco);
            offset = inode->i_size;
           // pthread_rwlock_unlock(&inode->trinco);
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            return -1; // no space in directory
        }

        offset = 0;
    } else {
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
    if(!valid_pathname(link_name)){
        return -1;
    }
    //Check if softlink points to itself
    if(strcmp(target,link_name) == 0){
        return -1;
    }
    //Check if name is already in use
    if(find_in_dir(inode_get(ROOT_DIR_INUM),link_name + 1) != -1){
        return -1;
    }
    int numsym = inode_create(T_SYMLINK);
        if (numsym == -1) {
            return -1; // no space in inode table
        }
    inode_t * symlink = inode_get(numsym);
    ALWAYS_ASSERT(symlink != NULL, "tfs_write: inode of open file deleted");
   // pthread_rwlock_wrlock(&symlink->trinco);
    strcpy(symlink->sympath,target);
   // pthread_rwlock_unlock(&symlink->trinco);
    if (add_dir_entry(inode_get(ROOT_DIR_INUM),link_name + 1,numsym) == -1){
        inode_delete(numsym);
        return -1; // no space in directory
    }
    return 0;

}

int tfs_link(char const *target, char const *link_name) {
     inode_t * r = inode_get(ROOT_DIR_INUM);
    if(!valid_pathname(link_name)){
            return -1;
    }
    //Check if target and link name are the same
    if(strcmp(target,link_name) == 0){
        return -1;
    }
    //Check if name is already in use
    if(find_in_dir(inode_get(ROOT_DIR_INUM),link_name + 1) != -1){
        return -1;
    }
    int inumber = find_in_dir(r,target + 1);
    if(inumber == -1){
        return -1; //entry not found
    }
    inode_t * node  = inode_get(inumber);
    ALWAYS_ASSERT(node != NULL, "tfs_write: inode of open file deleted");
    if(node->i_node_type == T_SYMLINK){
            return -1;
    }
    //pthread_rwlock_wrlock(&node->trinco);
    node->count++;
    //pthread_rwlock_unlock(&node->trinco);
    if (add_dir_entry(r,link_name + 1,inumber) == -1){
        inode_delete(inumber);
        return -1; // no space in directory
    }
    return 0;

}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    //  From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");
    // Determine how many bytes to write
    size_t block_size = state_block_size();
    //pthread_mutex_lock(&trinco2);
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }
    //pthread_mutex_unlock(&trinco2);
    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                return -1; // no space
            }
            //pthread_rwlock_wrlock(&inode->trinco);
            inode->i_data_block = bnum;
           // pthread_rwlock_unlock(&inode->trinco);
        }
        //pthread_rwlock_rdlock(&inode->trinco);
        void *block = data_block_get(inode->i_data_block);
       // pthread_rwlock_unlock(&inode->trinco);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
       // pthread_rwlock_wrlock(&inode->trinco);
        memcpy(block + file->of_offset, buffer, to_write);
        //pthread_rwlock_unlock(&inode->trinco);
        // The offset associated with the file handle is incremented accordingly
       //pthread_mutex_lock(&trinco2);
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
        //pthread_mutex_unlock(&trinco2);
    }
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }
    // From the open file table entry, we get the inode
    //pthread_mutex_lock(&trinco2);
    inode_t *inode = inode_get(file->of_inumber);

    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read

    size_t to_read = inode->i_size - file->of_offset;
    //pthread_mutex_unlock(&trinco2);
    if (to_read > len) {
        to_read = len;
    }
    if (to_read > 0) {
      //  pthread_rwlock_rdlock(&inode->trinco);
        void *block = data_block_get(inode->i_data_block);
       // pthread_rwlock_unlock(&inode->trinco);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");
        // Perform the actual read
       // pthread_rwlock_wrlock(&inode->trinco);
        memcpy(buffer, block + file->of_offset, to_read);
       // pthread_rwlock_unlock(&inode->trinco);
        // The offset associated with the file handle is incremented accordingly
       // pthread_mutex_lock(&trinco2);
        file->of_offset += to_read;
       // pthread_mutex_unlock(&trinco2);
    }

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    if (!valid_pathname(target)) {
        return -1;
    }
    //Get root inode
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    //Get inum of target to unlink
    int inum = tfs_lookup(target, root_dir_inode);
    if(inum >=0){
        inode_t *inode = inode_get(inum);
        if(inode->i_node_type == T_SYMLINK){
            clear_dir_entry(root_dir_inode,target + 1);
            inode_delete(inum);
            return 0;
        }else if(inode->i_node_type == T_DIRECTORY){
            return -1;
        }
        else{
            if(in_open_file_table(target + 1) == 0 && inode->count == 1){
                return -1;
            }
            clear_dir_entry(root_dir_inode, target + 1);
           // pthread_rwlock_wrlock(&inode->trinco);
            inode->count--;
            //pthread_rwlock_unlock(&inode->trinco);
            if (inode->count == 0) {
                inode_delete(inum);
            }
            return 0;
        }

    }
    return -1; //file or link doesn't exist
    }
    //se tentarmos dar unlink de  um ficheiro aberto, dar erro, e fazer teste a mostrar esse erro, e comentar o codigo


int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
         if(!valid_pathname(dest_path)) {
             return -1;
         }
         if(find_in_dir(inode_get(ROOT_DIR_INUM),source_path)){

         }
         //Open file inside TFS, either creating a new one or removing its contents
        int dest = tfs_open(dest_path, TFS_O_CREAT|TFS_O_TRUNC);
         //Open file outside TFS
        FILE * initial = fopen(source_path,"r");
        if (initial == NULL){
            return -1;
        }
        char buffer[BUFFER_LEN];
        memset(buffer,0,sizeof(buffer));
        //Read outside file into buffer
        size_t read = fread(buffer, sizeof(char), strlen(buffer) + 1, initial);
        //Repeat until nothing else is read
        while(read > 0){
            tfs_write(dest,buffer, strlen(buffer));
            memset(buffer,0,sizeof(buffer));
            read = fread(buffer, sizeof(char), strlen(buffer) + 1, initial);
        }
        tfs_close(dest);
        fclose(initial);
        return 0;
    }
