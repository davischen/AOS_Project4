#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
//#include "mman.h"
#include "proc.h"

#define NULL (mmap_region*)0


static void remove_region(mmap_region* node, mmap_region* prev)
{
  if(node == myproc()->region_head)
  {
    if(myproc()->region_head->next != 0)
      myproc()->region_head = myproc()->region_head->next;
    else
      myproc()->region_head = 0;
  }
  else
    prev->next = node->next;
  kmfree(node);
}
static void free_region(struct proc *curproc ,void *addr, uint length)
{
  if (curproc->region_head == NULL) {
    return;
  }

  mmap_region* curnode = curproc->region_head;
  mmap_region* next_node = curproc->region_head->next;

  //iterate over the list to check if the parameters passed to munmap are in the list
  //reduce the process size, delete the node.
  while(next_node != 0)
  {
    if(next_node->addr == addr && next_node->length == length)
    {
      curproc->sz = deallocuvm(curproc->pgdir, curproc->sz, curproc->sz - length);
      switchuvm(curproc);
      curproc->nregions--;
      remove_region(next_node, curnode);
    }
    curnode = next_node;
    next_node = curnode->next;
  }
}
//#define DEBUG
//mmap is to create a new mapping in the calling process's address space
//addr - starting adress to place the new memory region
//length - length of the region to mmap
//prot - info flags for mapped region
//fd - ANONYMOUS
//fd - default -1
//offset 
// returns - starting address of maped region or -1 on failure
void *mmap(void *addr, uint length, int prot, int flags, int fd, int offset)
{
  // Check for invalid arguments
  if (length == 0)
    return (void*)-1;

  //it checks if the requested address is page-aligned
  if ((uint)addr % PGSIZE != 0) {
    return (void*)-1;
  }

  //get current point
  struct proc *curproc = myproc();
  //it gets the current process size
  uint oldsz = curproc->sz;
  //adds the requested length to the new size of the process
  uint newsz = curproc->sz + length;

  //allocuvm to extend memory size
  //extend the memory size of the process by the requested length.
  if((curproc->sz = allocuvm(curproc->pgdir, oldsz, newsz))==0)
     return (void*)-1;

  //read pagetable of current process
  switchuvm(curproc);

  //allocate new region memory
  mmap_region* new_region = (mmap_region*)kmalloc(sizeof(mmap_region));
  //if allocation fails
  if(new_region == NULL)
  {
    //to free up previously new process size
    deallocuvm(curproc->pgdir, newsz, oldsz);
    return (void*)-1;
  }

  //store the mapping information for the newly allocated memory region
  addr = (void*)PGROUNDDOWN(oldsz);
  new_region->addr = addr;
  new_region->length = length;
  new_region->region_type = flags;
  new_region->offset = offset;
  new_region->prot = prot;
  new_region->next = 0;

  // if it is the first region in current process, set header point to new region
  if(curproc->nregions == 0)
    curproc->region_head = new_region;
  // else iterate over the mapped list and check memory region in boundary of current process
  else
  {
    //it iterates over the mapped list and checks whether the memory region is in the boundary of the current process.
    mmap_region* curnode = curproc->region_head;
    while(curnode->next != 0)
    {
      //from starting address to iterate all mapped list
      if(addr == curnode->addr)
      {
        addr += PGROUNDDOWN(PGSIZE+length);
        curnode = curproc->region_head;
      }
      //check if addr is larger than upper limit (KERNBASE)
      else if(addr == (void*)KERNBASE || addr > (void*)KERNBASE)
      {
        //fail to allocate new region
        kmfree(new_region);
        deallocuvm(curproc->pgdir, newsz, oldsz);
        return (void*)-1;
      }
      //move to next until last region
      curnode = curnode->next;
    }
    //
    if (addr == curnode->addr)
      addr += PGROUNDDOWN(PGSIZE+length);
    curnode->next = new_region;
  }

  //the number of regions add 1 to increment region count
  curproc->nregions++;
  //assign new region's starting address
  new_region->addr = addr;
  return new_region->addr;  
}
int munmap(void *addr, uint length)
{
  if (addr == NULL ||  length == 0)
    return -1;

  //verify that the address and length passed is indeed a valid mapping
  //addr has to smaller than KERNBASE, and its length must be larger than 1
  if (addr == (void*)KERNBASE || addr > (void*)KERNBASE || length < 1)
  {
    return -1;
  }
  struct proc *curproc = myproc();
  
  if (curproc->nregions == 0)
    return -1;

  //If the region to be removed is the first and only region in the list of mapped regions, then it frees up the memory
  if (curproc->region_head->addr == addr && curproc->region_head->length == length)
  {
    curproc->sz = deallocuvm(curproc->pgdir, curproc->sz, curproc->sz - length);
    switchuvm(curproc);
    curproc->nregions--;
    remove_region(curproc->region_head, 0);
    return 0;
  }
  else{
     //iterates over the list of mapped regions and finds the node corresponding to the given address and length.
     free_region(curproc,addr,length);
  }
  return 0;
}