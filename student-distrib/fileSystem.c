/* fileSystem.c - Defines for the file system's open, close, read and write
 * created by YMH, LYS on 2022/10/22
*/

#include "fileSystem.h"
#include "lib.h"


/* !!!every passed in file name must be exact 32 bytes with zero padding!!! (write a transform function to do that) */


#define BLOCK_SIZE 4096  //block size in bytes
#define FNAME_LEN 32  //max length of file name in bytes
#define DENTRY_SIZE 64  //file system dentry size in bytes
#define FTYPE_OFF 32  //offset to get file type field
#define INODE_OFF 36  //offset to get inode field


static uint8_t* fileSys_addr;
static bootBlock_t bootBlock;
dentry_t f_dentry;
dentry_t d_dentry;
static fdInfo_t Temp_fd_array[8];  // A temporary global array for file descriptors just for CheckPoint 2
uint8_t all_fname_list[17][32]; // contain the name of all 17 files, foe testing purpose, fill at process list all files


/******************* file system module *******************/

/*init_fileSys
 * description: load the start addr of file system into static variable fileSys_addr and init bootBlock
 * return value: 0 on success, -1 on file not found
 */
void init_fileSys(uint32_t* filesys_addr) {
    fileSys_addr = (uint8_t *)filesys_addr;
    bootBlock.num_dentries = *fileSys_addr;
    bootBlock.num_inodes = *(fileSys_addr+4);
    bootBlock.num_data_blocks = *(fileSys_addr+8);
}

/*read_dentry_by_name
 * description: Search file by file name and put its file name, type and inode into dentry
 * return value: 0 on success, -1 on file not found
 */
int32_t read_dentry_by_name (uint8_t* fname, dentry_t* dentry) {
    uint32_t i = 0;
    uint8_t* cur_fname;
    
    if (fname == NULL || dentry == NULL)
        return -1;

    while (i<bootBlock.num_dentries) {
        cur_fname = (fileSys_addr+(i+1)*DENTRY_SIZE);
        if (!strncmp(fname, cur_fname, 32)) break;
        i ++;
    }
    if (i==bootBlock.num_dentries) return -1;
    strncpy(dentry->file_name, cur_fname, 32);
    dentry->file_type = *(fileSys_addr+i+FTYPE_OFF);
    dentry->inode_index = *(fileSys_addr+i+INODE_OFF);

    return 0;
}

/*read_dentry_by_index
 * description: Search file by file index and put its file name, type and inode into dentry
 * return value: 0 on success, -1 on file not found
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry) {
    uint32_t entry_ptr = (index+1)*DENTRY_SIZE;
    if (index >= bootBlock.num_dentries) return -1;
    strncpy(dentry->file_name, (fileSys_addr+entry_ptr), 32);
    dentry->file_type = *(fileSys_addr+entry_ptr+FTYPE_OFF);
    dentry->inode_index = *(fileSys_addr+entry_ptr+INODE_OFF);
    return 0;
}

/*read_data
 * description: read length bytes of the file starting at offset into the buffer
 * return value: number of bytes read, or -1 if the inode number is invalid
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
    uint32_t N, D, inode_length, off_dblock, off_indblock, dblock_id, count=0;
    uint32_t *inode_ptr;
    uint8_t *byte_to_copy;

    N = bootBlock.num_inodes;
    D = bootBlock.num_data_blocks;
    inode_ptr = (inode+1)*BLOCK_SIZE + fileSys_addr;

    // check if inode is valid
    if (inode>=N) return -1;
    // check if offset >= inode_length
    inode_length = *inode_ptr;
    if (offset >= inode_length) return 0;

    //now start the read-data loop. remember to check the validity of data block number on each new data block
    off_dblock = offset/BLOCK_SIZE;
    off_indblock = offset - off_dblock*BLOCK_SIZE;
    dblock_id = *(inode_ptr+1+off_dblock);
    if (dblock_id>=D) return -1;
    while ((off_dblock*BLOCK_SIZE+off_indblock<inode_length) && (count<length)) {
        byte_to_copy = fileSys_addr + (N+dblock_id+1)*BLOCK_SIZE + off_indblock;
        strncpy(buf+count, byte_to_copy, 1);
        count++;
        off_indblock++;
        if (off_indblock==BLOCK_SIZE) {
            off_indblock=0;
            off_dblock ++;
            dblock_id = *(inode_ptr+1+off_dblock);
            if (dblock_id>=D) return -1;
        }
    }
    return count;
}


/******************* file operations *******************/

/*
 *  file_read:
 *    DESCRIPTION: read n bytes into buffer
 *    INPUTS: fd: file descriptor
 *            buf: buffer to store the data
 *            nbytes: number of bytes to be read
 *    RETURN VALUE: number of successfully read bytes
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {

    // memset((uint8_t*)buf, NULL, nbytes);

    uint32_t inode =  Temp_fd_array[fd].inode_index;
    uint32_t offset = Temp_fd_array[fd].file_position;
    uint32_t read_nbytes = read_data(inode, offset, (uint8_t*)buf, nbytes);

    if (read_nbytes == -1)
        return -1; // invalid input, read_data failed

    // if succeed, update file position
    Temp_fd_array[fd].file_position += read_nbytes;

    return read_nbytes;
}


/*
 *  file_write:
 *    DESCRIPTION: return -1 (since file is read-only)
 *    INPUTS: fd: file descriptor
 *            buf: buffer to store the data
 *            nbytes: number of bytes to be read
 *    RETURN VALUE: -1
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1;
}


/*
 *  file_write:
 *    DESCRIPTION: read corresponding dentry by the file name.
 *    INPUTS: filename: file name
 *            fd: file descriptor
 *    RETURN VALUE: 0 on success, -1 on file not found
 */
int32_t file_open(const uint8_t* filename, int fd) {

    fdInfo_t fd_info = Temp_fd_array[fd];
    int check = read_dentry_by_name((uint8_t*)filename, &f_dentry);
    if (check == 0)
    {
        fd_info.inode_index = f_dentry.inode_index;
        fd_info.flags = 1;
    }

    return check;
    
}


/*
 *  file_close:
 *    DESCRIPTION: return 0
 *    INPUTS: fd: file descriptor
 *    RETURN VALUE: 0
 */
int32_t file_close(int32_t fd){
    return 0;
}


/**************** directory operations ****************/

/*
 *  dir_read:
 *    DESCRIPTION: read a directory
 *    INPUTS: fd: file descriptor
 *            buf: buffer to store the data
 *            nbytes: number of bytes to be read
 *    RETURN VALUE: 0 on success, -1 on fail
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes) {

    // index for dentry currently being read
    uint32_t index =  Temp_fd_array[fd].file_position;

    int32_t check = read_dentry_by_index(index, &d_dentry);

    if(check == 0)
        strncpy((int8_t*)buf, (int8_t*)d_dentry.file_name, 32);
    else
        return -1;

    Temp_fd_array[fd].file_position += 1;

    return 0;
}


/*
 *  dir_write:
 *    DESCRIPTION: return -1
 *    INPUTS: fd: file descriptor
 *            buf: buffer to store the data
 *            nbytes: number of bytes to be read
 *    RETURN VALUE: -1
 */
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1;
}


/*
 *  dir_open:
 *    DESCRIPTION: open a directory
 *    INPUTS: filename: file name
 *            fd: file descriptor
 *    RETURN VALUE: 0 on success, -1 on fail
 */
int32_t dir_open(const uint8_t* filename) {

    return read_dentry_by_name((uint8_t*)filename, &d_dentry);

}


/*
 *  dir_close:
 *    DESCRIPTION: return 0
 *    INPUTS: fd: file descriptor
 *    RETURN VALUE: 0
 */
int32_t dir_close(int32_t fd) {
    return 0;
}






/**********************************************************************/
/*                                                                    */
/*                           Testing purpose                          */
/*                                                                    */
/**********************************************************************/

/*
 *  list_all_files:
 *    DESCRIPTION: list all files in the system, for testing purpose
 *    INPUTS: void
 *    RETURN VALUE: void
 */
void list_all_files() {
    int f_idx, ret, name_area=35, fn_len, i;
    dentry_t file;
    uint32_t *inode_ptr;
    uint8_t name_buf[33];
    name_buf[32]=0;
    for (f_idx = 0; f_idx<bootBlock.num_dentries; f_idx++) {
        ret = read_dentry_by_index(f_idx, &file);
        if (ret==-1) {
            printf("read by index return -1 on file %d", f_idx);
            return;
        }
        // print file properties: name, type and size
        fn_len = strlen(file.file_name);
        printf("file_name:");
        for (i=0; i<name_area-fn_len; i++) printf(" ");
        strncpy(name_buf, file.file_name, 32);
        strncpy(all_fname_list[f_idx], file.file_name, 32);
        printf(name_buf);
        printf(", ");
        printf("file_type: %d", file.file_type);
        printf(", ");
        inode_ptr = fileSys_addr+(file.inode_index + 1)*BLOCK_SIZE;
        printf("file_size: %d\n", *inode_ptr);
    }
}


// fill all_fname_list, testing purpose
void fill_fname_list() {
    int f_idx, ret, name_area=35, fn_len, i;
    dentry_t file;
    for (f_idx = 0; f_idx<bootBlock.num_dentries; f_idx++) {
        ret = read_dentry_by_index(f_idx, &file);
        if (ret==-1) {
            printf("read by index return -1 on file %d", f_idx);
            return;
        }
        strncpy(all_fname_list[f_idx], file.file_name, 32);
    }
}


// read_and_print_all_files, testing purpose
void list_all_files_by_name() {
    int f_idx, ret, name_area=35, fn_len, i;
    dentry_t file;
    uint32_t *inode_ptr;
    uint8_t name_buf[33];
    name_buf[32]=0;

    fill_fname_list();
    for (f_idx = 0; f_idx<bootBlock.num_dentries; f_idx++) {
        ret = read_dentry_by_name(all_fname_list[f_idx], &file);
        if (ret==-1) {
            printf("read by name return -1 on file %d", f_idx);
            return;
        }
        // print file properties: name, type and size
        fn_len = strlen(file.file_name);
        printf("file_name:");
        for (i=0; i<name_area-fn_len; i++) printf(" ");
        strncpy(name_buf, file.file_name, 32);
        strncpy(all_fname_list[f_idx], file.file_name, 32);
        printf(name_buf);
        printf(", ");
        printf("file_type: %d", file.file_type);
        printf(", ");
        inode_ptr = fileSys_addr+(file.inode_index + 1)*BLOCK_SIZE;
        printf("file_size: %d\n", *inode_ptr);
    }
}


// read the content of the ith file and print it on screen. notice: fish_id=10 (fish is an exe), very large=11, .exe(ls)=12, exe(hello)=16
void read_file_i(int f_idx) {
    int ret, f_size, i;
    dentry_t file;
    uint32_t inode_num;
    uint32_t *inode_ptr;
    uint32_t buf_size = 1024;
    uint8_t buf[buf_size]; // a buffer of 1KB
    uint32_t bytes_read, count=0, new_line=0;

    for (i=0; i<buf_size; i++) {
        buf[i] = 0;
    }

    ret = read_dentry_by_index(f_idx, &file);
    if (ret==-1) {
        printf("read by index return -1 on file %d", f_idx);
        return;
    }
    inode_num = file.inode_index;
    inode_ptr = fileSys_addr+(file.inode_index + 1)*BLOCK_SIZE;
    f_size = *inode_ptr;

    while (f_size > 0) {
        if (f_size<buf_size) bytes_read = read_data(inode_num, count, buf, f_size);
        else bytes_read = read_data(inode_num, count, buf, buf_size);
        if (bytes_read==-1) {
            printf("read data failed at offset %d", count);
            return;
        }
        f_size -= bytes_read;
        count += bytes_read;
        i = -1;
        while (++i<buf_size) {
            if (buf[i]==0) continue;
            new_line++;
            if (buf[i]=='\n') new_line=0;
            if (new_line>=80) {putc('\n'); new_line=0;}
            putc(buf[i]);
        }
    }

    return;
}