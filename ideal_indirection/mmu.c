/**
 * ideal_indirection
 * CS 241 - Fall 2021
 */


//Partner: mingw4, shunl2, zhichao8, qz13
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


void mmu_tlb_miss(mmu *this) {
    this->num_tlb_misses++;
}

void mmu_raise_page_fault(mmu *this) {
    this->num_page_faults++;
}

void mmu_raise_segmentation_fault(mmu *this) {
    this->num_segmentation_faults++;
}

void mmu_add_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    addr32 page_directory_address = ask_kernel_for_frame(NULL);
    this->page_directories[pid] =
        (page_directory *)get_system_pointer_from_address(
            page_directory_address);
    page_directory *pd = this->page_directories[pid];
    this->segmentations[pid] = calloc(1, sizeof(vm_segmentations));
    vm_segmentations *segmentations = this->segmentations[pid];

    // Note you can see this information in a memory map by using
    // cat /proc/self/maps
    segmentations->segments[STACK] =
        (vm_segmentation){.start = 0xBFFFE000,
                          .end = 0xC07FE000, // 8mb stack
                          .permissions = READ | WRITE,
                          .grows_down = true};

    segmentations->segments[MMAP] =
        (vm_segmentation){.start = 0xC07FE000,
                          .end = 0xC07FE000,
                          // making this writeable to simplify the next lab.
                          // todo make this not writeable by default
                          .permissions = READ | EXEC | WRITE,
                          .grows_down = true};

    segmentations->segments[HEAP] =
        (vm_segmentation){.start = 0x08072000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[BSS] =
        (vm_segmentation){.start = 0x0805A000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[DATA] =
        (vm_segmentation){.start = 0x08052000,
                          .end = 0x0805A000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[TEXT] =
        (vm_segmentation){.start = 0x08048000,
                          .end = 0x08052000,
                          .permissions = READ | EXEC,
                          .grows_down = false};

    // creating a few mappings so we have something to play with (made up)
    // this segment is made up for testing purposes
    segmentations->segments[TESTING] =
        (vm_segmentation){.start = PAGE_SIZE,
                          .end = 3 * PAGE_SIZE,
                          .permissions = READ | WRITE,
                          .grows_down = false};
    // first 4 mb is bookkept by the first page directory entry
    page_directory_entry *pde = &(pd->entries[0]);
    // assigning it a page table and some basic permissions
    pde->base_addr = (ask_kernel_for_frame(NULL) >> NUM_OFFSET_BITS);
    pde->present = true;
    pde->read_write = true;
    pde->user_supervisor = true;

    // setting entries 1 and 2 (since each entry points to a 4kb page)
    // of the page table to point to our 8kb of testing memory defined earlier
    for (int i = 1; i < 3; i++) {
        page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
        page_table_entry *pte = &(pt->entries[i]);
        pte->base_addr = (ask_kernel_for_frame(pte) >> NUM_OFFSET_BITS);
        pte->present = true;
        pte->read_write = true;
        pte->user_supervisor = true;
    }
}

void mmu_remove_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    // example of how to BFS through page table tree for those to read code.
    page_directory *pd = this->page_directories[pid];
    if (pd) {
        for (size_t vpn1 = 0; vpn1 < NUM_ENTRIES; vpn1++) {
            page_directory_entry *pde = &(pd->entries[vpn1]);
            if (pde->present) {
                page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
                for (size_t vpn2 = 0; vpn2 < NUM_ENTRIES; vpn2++) {
                    page_table_entry *pte = &(pt->entries[vpn2]);
                    if (pte->present) {
                        void *frame = (void *)get_system_pointer_from_pte(pte);
                        return_frame_to_kernel(frame);
                    }
                    remove_swap_file(pte);
                }
                return_frame_to_kernel(pt);
            }
        }
        return_frame_to_kernel(pd);
    }

    this->page_directories[pid] = NULL;
    free(this->segmentations[pid]);
    this->segmentations[pid] = NULL;

    if (this->curr_pid == pid) {
        tlb_flush(&(this->tlb));
    }
}

void mmu_delete(mmu *this) {
    for (size_t pid = 0; pid < MAX_PROCESS_ID; pid++) {
        mmu_remove_process(this, pid);
    }

    tlb_delete(this->tlb);
    free(this);
    remove_swap_files();
}