
/* PCB.c - PCB methods
 * created by LYS, JJL on 2022/10/29
 */

#include "PCB.h"

PCB_t *pid_table[MAX_PNUM];

/**
 * drush8: set the PCB table entry to be 0.
 * 
 */
//void init_PCBtable(){
//    int i;
//    for (i=0;i<MAX_PNUM;i++){
//        pid_table[i] = NULL;
//    }
    //needn't return, always success.
//}


//this one get the current task's pcb value (only when in kernel!). Warning: not search from the pid_table
PCB_t * get_PCB() {
    register uint32_t save_esp asm("esp"); 
    return (PCB_t*)((save_esp-1) & MASK);       //fix ebp to esp: drush8
}


//define the location of the PCB with calculation. Cannot make sure if this pid is in used...
PCB_t * get_PCB_withpid(int pid) { 
    return (PCB_t*)(8*MB-(pid+1)*8*KB);       //fix ebp to esp: drush8
}


//init the PCB structure: set all FD unused, and set self pid
//don'y set parent's, and don't open stdin/out for you!
int init_PCB(int pid) {
    int i;
    if(pid<0 || pid>=MAX_PNUM) return 1;

    pid_table[pid]->pid = pid;
    pid_table[pid]->parent_pid = 0; //by default we set it to 0..
    for (i=0;i<MAX_FD;i++){
        pid_table[pid]->fd_array[i].flags = 0; //all fd is unused.
    }

    pid_table[pid]->kebp = NULL;
    pid_table[pid]->kesp = NULL;
    return 0;
}


//called when booting, initialize pid_table, and set the PCB for task 0.
void  init_Syscall() {
    int i;
    for (i=0; i<MAX_PNUM; i++) {
        pid_table[i]=NULL;
    }
    PCB_t *PCB_prime = (PCB_t *) (8*MB - 8*KB);
    pid_table[0] = PCB_prime;
    init_PCB(0);
}
