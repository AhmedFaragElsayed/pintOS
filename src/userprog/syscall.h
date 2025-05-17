#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include <string.h>
#define BYTE 1
#define CODE_SEG_ADDR (void*)0x08048000
#define VALID_ADDRESS(VADDR) ({VADDR>CODE_SEG_ADDR && VADDR<PHYS_BASE;})

typedef int pid_t;

void syscall_init (void);

/*VADDR VALIDATIONS*/
void* get_next_arg(void**, const unsigned);
void validate_vaddr(const void*);
void validate_buffer(const void*, const unsigned);
void validate_string(char*);


/* SYS CALLS WRAPPER*/
void sys_halt_wrapper (void);
void sys_exit_wrapper (void*);
void sys_exec_wrapper (void*, uint32_t*);
void sys_wait_wrapper (void*, uint32_t*);
void sys_create_wrapper (void*, uint32_t*);
void sys_remove_wrapper (void*, uint32_t*);
void sys_open_wrapper (void*, uint32_t*);
void sys_filesize_wrapper (void*, uint32_t*);
void sys_read_wrapper (void*, uint32_t*);
void sys_write_wrapper (void*, uint32_t*);
void sys_seek_wrapper (void*);
void sys_tell_wrapper (void*, uint32_t*);
void sys_close_wrapper (void*);

/* SYS CALLS*/
void sys_halt (void);
void sys_exit (int);
pid_t sys_exec (const char*);
int sys_wait (pid_t);
bool sys_create (const char*, unsigned);
bool sys_remove (const char*);
int sys_open (const char*);
int sys_filesize (int);
int sys_read (int, void*, unsigned);
int sys_write (int, const void*, unsigned);
void sys_seek (int, unsigned);
unsigned sys_tell (int);
void sys_close (int);

#endif /* userprog/syscall.h */
