/**
 * ideal_indirection
 * CS 241 - Fall 2021
 */


//Partner: mingw4, shunl2, zhichao8
#include "mmu.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static uintptr_t bass_addr;
static uintptr_t first_addr;
static uintptr_t middle_addr;
static uintptr_t final_addr;

mmu *mmu_create() {
    mmu *my_mmu = calloc(1, sizeof(mmu));
    my_mmu->tlb = tlb_create();
    return my_mmu;
}

void helper2(void* temp, int m){
    page_table_entry* pd_entry = (page_table_entry*)temp;
    pd_entry->base_addr = ask_kernel_for_frame(NULL) >> 12;
    pd_entry->present = 1;
    if(m) {
        pd_entry->read_write = 1;
        pd_entry->user_supervisor = 1;
    }
    read_page_from_disk((page_table_entry*)pd_entry);
}


page_table_entry* helper(mmu *this, addr32 virtual_address, size_t pid, int m){
    assert(this);
    assert(pid < MAX_PROCESS_ID);

    // check address is valid and permission not deny
    int check_address = address_in_segmentations(this->segmentations[pid], virtual_address);
    vm_segmentation* segment = find_segment(this->segmentations[pid], virtual_address);
    int check = segment->permissions & READ;
    if(m) check = segment->permissions & WRITE;
    if (!check_address || !check) {
        mmu_raise_segmentation_fault(this);
        return NULL;
    }

    // check for correct pid input
    if (this->curr_pid != pid) {
        tlb_flush(&this->tlb);
        this->curr_pid = pid;
    }
    // base virtual last 12 clear 0
    bass_addr = virtual_address >> 12;
    bass_addr = bass_addr << 12;

    // first 10
    first_addr = virtual_address >> 22;
    // middle 10
    middle_addr = virtual_address << 10;
    middle_addr = middle_addr >> 22;
    // last 12
    final_addr = virtual_address << 20;
    final_addr = final_addr >> 20;
   

    page_table_entry* pt_entry = tlb_get_pte(&this->tlb, bass_addr);

    if (pt_entry) {
        // if not present we set to disk
        if (!pt_entry->present) {
            mmu_raise_page_fault(this);
            helper2(pt_entry,0);
        }
        pt_entry->accessed = 1;   
    } else {
        mmu_tlb_miss(this);
        page_directory* page_dir = this->page_directories[pid];
        page_directory_entry *pd_entry = &page_dir->entries[first_addr];
        // if not present we set to disk
        if (!pd_entry->present) {
            mmu_raise_page_fault(this);
            helper2(pd_entry,1);
        }
        page_table *page_tab = (page_table*)get_system_pointer_from_pde(pd_entry);
        pt_entry = &(page_tab->entries[middle_addr]);
        pt_entry->accessed = 1;
    } 
    return pt_entry;
}


void mmu_read_from_virtual_address(mmu *this, addr32 virtual_address,size_t pid, void *buffer, size_t num_bytes) {
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
    page_table_entry* pt_entry = helper(this,virtual_address,pid,0);
    if(pt_entry){
        memcpy(buffer, get_system_pointer_from_pte(pt_entry) + final_addr, num_bytes);
        tlb_add_pte(&this->tlb, bass_addr, pt_entry);
    }
    
}

void mmu_write_to_virtual_address(mmu *this, addr32 virtual_address, size_t pid,const void *buffer, size_t num_bytes) {
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
    page_table_entry* pt_entry = helper(this,virtual_address,pid,1);
    if(pt_entry){
        pt_entry->dirty = 1;
        memcpy(get_system_pointer_from_pte(pt_entry) + final_addr,buffer, num_bytes);
        tlb_add_pte(&this->tlb, bass_addr, pt_entry);
    }
}