/* fileSystem.c - Defines for the file system's open, close, read and write
 * created by YMH, LYS on 2022/10/22
*/

#include "fileSystem.h"
#include "lib.h"
#include "Syscalls.h"


/* !!!every passed in file name must be exact 32 bytes with zero padding!!! (write a transform function to do that) */


uint8_t* fileSys_addr;
bootBlock_t bootBlock;

/* contain the name of all 17 files (each file name size is 32), foe testing purpose, fill at process list all files */
uint8_t all_fname_list[17][32];


/******************* file system module *******************/

/*init_fileSys
 * description: load the start addr of file system into static variable fileSys_addr and init bootBlock
 * return value: 0 on success, -1 on file not found
 */
void init_fileSys(uint32_t* filesys_addr) {
    fileSys_addr = (uint8_t *)filesys_addr;
    bootBlock.num_dentries = *fileSys_addr;
    bootBlock.num_inodes = *(fileSys_addr+4);   // offset 4 to get inodes number
    bootBlock.num_data_blocks = *(fileSys_addr+8);  // offset 8 to get data block number
}

/*read_dentry_by_name
 * description: Search file by file name and put its file name, type and inode into dentry
 * return value: 0 on success, -1 on file not found
 */
int32_t read_dentry_by_name (uint8_t* fname, dentry_t* dentry) {
    uint32_t i = 0, copy_len;
    uint8_t* cur_fname;
    uint8_t fname_pad[FNAME_LEN];
    
    if (fname == NULL || dentry == NULL) {
        return -1;
    }

    //pad fname to 32 bytes for comparison
    for (i=0; i<FNAME_LEN; i++) fname_pad[i]=0;
    copy_len = strlen((int8_t*)fname);
    if (copy_len > FNAME_LEN) copy_len=FNAME_LEN;
    strncpy((int8_t*)fname_pad, (int8_t*)fname, copy_len);

    i=0;
    while (i<bootBlock.num_dentries) {
        cur_fname = (fileSys_addr+(i+1)*DENTRY_SIZE);
        if (!strncmp((int8_t*)fname_pad, (int8_t*)cur_fname, FNAME_LEN)) break;
        i++;
    }
    if (i==bootBlock.num_dentries) {
        return -1;
    }
    strncpy((int8_t*)dentry->file_name, (int8_t*)cur_fname, FNAME_LEN);

    dentry->file_type = *(fileSys_addr+(i+1)*DENTRY_SIZE+FTYPE_OFF);
    dentry->inode_index = *(fileSys_addr+(i+1)*DENTRY_SIZE+INODE_OFF);

    return 0;
}

/*read_dentry_by_index
 * description: Search file by file index and put its file name, type and inode into dentry
 * return value: 0 on success, -1 on file not found
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry) {
    uint32_t entry_ptr = (index+1)*DENTRY_SIZE;
    if (index >= bootBlock.num_dentries) return -1;
    strncpy((int8_t*)dentry->file_name, (int8_t*)(fileSys_addr+entry_ptr), FNAME_LEN);
    dentry->file_type = *(fileSys_addr+entry_ptr+FTYPE_OFF);
    dentry->inode_index = *(fileSys_addr+entry_ptr+INODE_OFF);
    return 0;
}

/*read_data
 * description: read length bytes of the file starting at offset into the buffer
 * return value: number of bytes read, or -1 if the inode number is invalid, or 0 if offset >= inode_length
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
    uint32_t N, D, inode_length, off_dblock, off_indblock, dblock_id, count=0;
    uint32_t *inode_ptr;
    uint8_t *byte_to_copy;

    N = bootBlock.num_inodes;
    D = bootBlock.num_data_blocks;
    inode_ptr = (uint32_t*)((inode+1)*BLOCK_SIZE + fileSys_addr);

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
        strncpy((int8_t*)buf+count, (int8_t*)byte_to_copy, 1);  // read 1 byte each time
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
    PCB_t * PCB_current = get_PCB();
    /* get inode index and offset from fd array */
    uint32_t inode = PCB_current->fd_array[fd].inode_index;
    uint32_t offset = PCB_current->fd_array[fd].file_position;

    uint32_t read_nbytes = read_data(inode, offset, (uint8_t*)buf, nbytes);

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
 *  file_open:
 *    DESCRIPTION: return 0
 *    INPUTS: filename: file name
 *    RETURN VALUE: 0 on success, -1 on file not found
 */
int32_t file_open(const uint8_t* filename) {
    return 0;
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
 *    RETURN VALUE: 0 on run out of the file reading. or the length of the cur file's name
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes) {
    PCB_t * PCB_current = get_PCB();
    dentry_t d_dentry;

    // get the index of corresponding dentry from fd array
    uint32_t index = PCB_current->fd_array[fd].file_position;

    int32_t check = read_dentry_by_index(index, &d_dentry);

    if(check == 0)
        strncpy((int8_t*)buf, (int8_t*)d_dentry.file_name, FNAME_LEN);
    else
        return 0;//no files anymore...
    int length = strlen((int8_t *)d_dentry.file_name);
    if (length<=32) return length;
    return 32;   // update the index in file positon to read next file
//drush8's works.


    // /* print out the file information */
    // int name_area=35, fn_len, i;    // total name area size is 35 in our design
    // uint32_t *inode_ptr;
    // // print file properties: name, type and size
    // fn_len = strlen((int8_t*)d_dentry.file_name);
    // if (fn_len > FNAME_LEN)
    //     fn_len = FNAME_LEN;
    // printf("file_name:");
    // for (i=0; i<name_area-fn_len; i++) printf(" ");
    // printf((int8_t*)buf);
    // printf(", ");
    // printf("file_type: %d", d_dentry.file_type);
    // printf(", ");
    // inode_ptr = (uint32_t*)(fileSys_addr+(d_dentry.inode_index + 1)*BLOCK_SIZE);
    // printf("file_size: %d\n", *inode_ptr);

    // return 0;
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
 *    DESCRIPTION: open a directory (return 0)
 *    INPUTS: filename: file name
 *            fd: file descriptor
 *    RETURN VALUE: 0 on success, -1 on fail
 */
int32_t dir_open(const uint8_t* filename) {
    return 0;
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
/*                           Testing Functions                        */
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
        fn_len = strlen((int8_t*)file.file_name);
        printf("file_name:");
        for (i=0; i<name_area-fn_len; i++) printf(" ");
        strncpy((int8_t*)name_buf, (int8_t*)file.file_name, 32);
        strncpy((int8_t*)all_fname_list[f_idx], (int8_t*)file.file_name, 32);
        printf((int8_t*)name_buf);
        printf(", ");
        printf("file_type: %d", file.file_type);
        printf(", ");
        inode_ptr = (uint32_t*)(fileSys_addr+(file.inode_index + 1)*BLOCK_SIZE);
        printf("file_size: %d\n", *inode_ptr);
    }
}


// fill all_fname_list, testing purpose
void fill_fname_list() {
    int f_idx, ret;
    dentry_t file;
    for (f_idx = 0; f_idx<bootBlock.num_dentries; f_idx++) {
        ret = read_dentry_by_index(f_idx, &file);
        if (ret==-1) {
            printf("read by index return -1 on file %d", f_idx);
            return;
        }
        strncpy((int8_t*)all_fname_list[f_idx], (int8_t*)file.file_name, 32);
    }
}


// read_and_print_all_files, testing purpose
void list_all_files_by_name() {
    int f_idx, ret, name_area=35, i;
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
        uint32_t fn_len = strlen((int8_t*)file.file_name);
        printf("file_name:");
        for (i=0; i<name_area-fn_len; i++) printf(" ");
        strncpy((int8_t*)name_buf, (int8_t*)file.file_name, 32);
        strncpy((int8_t*)all_fname_list[f_idx], (int8_t*)file.file_name, 32);
        printf((int8_t*)name_buf);
        printf(", ");
        printf("file_type: %d", file.file_type);
        printf(", ");
        inode_ptr = (uint32_t*)(fileSys_addr+(file.inode_index + 1)*BLOCK_SIZE);
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
    uint8_t buf[1024]; // a buffer of 1KB
    uint32_t bytes_read, count=0;
    //new_line=0;

    for (i=0; i<buf_size; i++) {
        buf[i] = 0;
    }

    ret = read_dentry_by_index(f_idx, &file);
    if (ret==-1) {
        printf("read by index return -1 on file %d", f_idx);
        return;
    }
    inode_num = file.inode_index;
    inode_ptr = (uint32_t*)(fileSys_addr+(file.inode_index + 1)*BLOCK_SIZE);
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
            //new_line++;
            //if (buf[i]=='\n') new_line=0;
            //if (new_line>=80) {putc('\n'); new_line=0;}
            putc(buf[i]);
        }
    }

    return;
}


/*
 * test file_open and file_read
 */
void file_OpenRead_test() {
    PCB_t * PCB_current = get_PCB();
    fill_fname_list();
    uint8_t* fname = all_fname_list[15];    // in this case: read "frame1.txt" (index is 15 in all_fname_list)

    /* set the file descriptor information */
	int fd = 1;
    PCB_current->fd_array[fd].inode_index = 0;
    PCB_current->fd_array[fd].file_position = 0;
    PCB_current->fd_array[fd].flags = 0;

	int check = file_open(fname);
	if (check == 0) {
        printf("Open ");
        printf((int8_t*)fname);
		printf(" successfully!\n");
    }
	else
		printf("file_open FAIL! \n");
	
	int32_t i, count;
    int new_line = 0;
	uint8_t buf[100000];    // set large enough number for buffer size
	count = file_read(fd, buf, 100000);
	// printf("Successfully read %d Bytes!\n", count);
    printf((int8_t*)fname);
    printf(":\n");
	for (i = 0; i < count; i++) {
        if (buf[i] != '\0') {
            putc(buf[i]);
            new_line++;
            if (buf[i]=='\n') new_line=0;
            if (new_line>=80) {putc('\n'); new_line=0;}     // number of display screen column is 80
        }
	}

	file_close(fd);

    return;
}


/*
 * test dir_open and dir_read
 */
void dir_OpenRead_test(int fd) {

	uint8_t buf[33];    // set larger number for buffer size (33 > 32)
	buf[32] = 0;    // set the last char to 0 since max file name size is 32

	dir_open(0);
	dir_read(fd, buf, 0);

	dir_close(fd);

    return;
}


/*
 * print the infomation for all files in the directory
 */
void Print_dir_test() {
    PCB_t * PCB_current = get_PCB();
    /* set the file descriptor information */
	int fd = 0;
    PCB_current->fd_array[fd].inode_index = 0;
    PCB_current->fd_array[fd].file_position = 0;
    PCB_current->fd_array[fd].flags = 0;

	int i;
	for (i = 0; i < 17; i++)    // in total 17 files
	{
		dir_OpenRead_test(fd);
	}

	return;
	
}
