/* x86_page.c - 
 * created by LYS on 2022/10/15
 */

#include "x86_desc.h"
#include "lib.h"
#include "x86_page.h"

#define VIDEO_MEM				0x000B8000
#define MASK_ADDR_FOR_PT		0x003FF000
#define PAGING_SIZE 			1024
PDE_t PD[1024] __attribute__((aligned (0x1000)));
PTE_t PT[1024] __attribute__((aligned (0x1000)));
PTE_t PT_user[1024] __attribute__((aligned (0x1000)));

/*1. Initialize PD[0] to point to 0~4MB as 4KB pages (PD[0] points to our current only page table PT), 
  PD[1] points to 4~8MB as a 4MB page, PT[0] points to video memory.
  2. Enable paging */
void init_paging() {
	int pt_vid_mem_entry = (VIDEO_MEM & MASK_ADDR_FOR_PT) >> 12;
	uint32_t i;
		// Initialize all the PTE and PDE to not present
	for (i = 0; i < PAGING_SIZE; i++) {
		PD[i].val = 0;
		PT[i].val = 0;
	}
    SET_PD_ENTRY_4K(PD[0], &PT[0], 0, 1);
    SET_PT_ENTRY(PT[pt_vid_mem_entry], 0xB8000, 0, 1);
    SET_PD_ENTRY_4M(PD[1], 0x400000, 0, 1);

	/* Initialize PD[1022] for dynamic allocated memory (4088~4092MB) */
	SET_PD_ENTRY_4M(PD[1022], 0xFF800000, 0, 1); 	// ymh

    asm volatile(
			 	"movl %0, %%eax;"
				"movl %%eax, %%cr3;"
				"movl %%cr4, %%eax;"
				"orl $0x010, %%eax;"
				"movl %%eax, %%cr4;"
				"movl %%cr0, %%eax;"
				"orl $0x80000000, %%eax;"
				"movl %%eax, %%cr0;"
				:
				: "r" (PD)
				: "eax"
	);                                 
}

//LYS: map vm addr to phys addr
void map_4M(uint32_t vm, uint32_t phys) {
	// uint32_t phys_;
	// if (PD[vm>>22].P) {
	// 	phys_ = (PD[vm>>22].PBase_Addr)<<22;
	// 	printf("can't map this, vm %#x has already been mapped to phys %#x\n", vm, phys_);
	// 	return;
	// }
	SET_PD_ENTRY_4M(PD[vm>>22], phys, 0, 1);
	Flush_TLB();
}

//drush8: map vm addr to phys addr for the user
void map_4M_U(uint32_t vm, uint32_t phys) {
	// uint32_t phys_;
	// if (PD[vm>>22].P) {
	// 	phys_ = (PD[vm>>22].PBase_Addr)<<22;
	// 	printf("can't map this, vm %#x has already been mapped to phys %#x\n", vm, phys_);
	// 	return;
	// }
	SET_PD_ENTRY_4M(PD[vm>>22], phys, 1, 1);
	Flush_TLB();
}

//LYS: unmap vm addr
void unmap_4M(uint32_t vm) {
	//check if vm already been unmapped
	if (!(PD[vm>>22].P)) {
		printf("can't unmap this, vm %#x is not presented", vm);
		return;
	}
	UNSET_PD_ENTRY(PD[vm>>22]);
	Flush_TLB();
}


//LYS: map vm addr to phys addr
void map_4K(uint32_t vm, uint32_t phys) {
	// uint32_t phys_;
	// if (PD[vm>>22].P && PT[(PD[vm>>22].PTBase_Addr)].P) {
	// 	phys_ = PT[(PD[vm>>22].PTBase_Addr)].PBase_Addr << 12;
	// 	printf("can't map this, vm %#x has already been mapped to phys %#x\n", vm, phys_);
	// 	return;
	// }
	SET_PD_ENTRY_4K(PD[vm>>22], &PT[0], 0, 1);
	SET_PT_ENTRY(PT[(vm>>12)&(0x3FF)], phys, 0, 1);
	Flush_TLB();
}

//LYS: map vm addr to phys addr for the user
void map_4K_U(uint32_t vm, uint32_t phys) {
	// uint32_t phys_;
	// if (PD[vm>>22].P && PT_user[(PD[vm>>22].PTBase_Addr)].P) {
	// 	phys_ = PT_user[(PD[vm>>22].PTBase_Addr)].PBase_Addr << 12;
	// 	printf("can't map this, vm %#x has already been mapped to phys %#x\n", vm, phys_);
	// 	return;
	// }
	SET_PD_ENTRY_4K(PD[vm>>22], &PT_user[0], 1, 1);
	SET_PT_ENTRY(PT_user[(vm>>12)&(0x3FF)], phys, 1, 1);
	Flush_TLB();
}

//LYS: unmap vm addr to phys addr
void unmap_4K(uint32_t vm) {
	//check if vm already been unmapped
	if (!(PD[vm>>22].P)) {
		printf("can't unmap this, vm %#x is not presented", vm);
		return;
	}

	PTE_t *PT_ = (PTE_t *)(((uint32_t)(PD[vm>>22].PTBase_Addr))<<12);
	int PT_addr = (vm&0x003FF000)>>12;
	UNSET_PT_ENTRY(PT_[PT_addr]);
	UNSET_PD_ENTRY(PD[vm>>22]);
	Flush_TLB();
}
