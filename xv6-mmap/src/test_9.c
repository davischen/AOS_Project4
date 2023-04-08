#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"
#include "mmu.h"

#define CHUNK_SIZE 4096

int
main(int argc, char *argv[])
{
  void *ptr = 0;
  int count = 0;
  char *addr = (char*)0x4000;
  while ((ptr = mmap(addr, CHUNK_SIZE, 0,0, -1, 0)) != 0 && count<=10) {
      count++;

      printf(1,"Allocated %d chunks of memory.\n", count);
  }

  // free all memory using munmap
  int i;
  for (i = 0; i < count; i++) {
      if (munmap(ptr + i*CHUNK_SIZE, CHUNK_SIZE) < 0) {
          printf(1,"munmap failed.\n");
          exit();
      }
  }
  printf(1,"Freed all memory.\n");
  return 0;
}