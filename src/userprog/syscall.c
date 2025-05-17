#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
static struct lock filesys_lock;

void*
get_next_arg(void** esp, unsigned arg_size)
{
  validate_buffer(*esp , arg_size);
  void* temp_esp = *esp;
  *esp += arg_size;
  return temp_esp;
}


void
validate_vaddr(const void* ptr)
{
  if(!ptr || !VALID_ADDRESS(ptr) || !pagedir_get_page(thread_current()->pagedir, ptr))
  {
    sys_exit(-1);
  }
}


void
validate_buffer(const void* buffer , const unsigned size)
{
  validate_vaddr(buffer);
  for(unsigned i = 0 ; i < size ; ++i)
  {
    validate_vaddr(buffer + BYTE*i);
  }
}

void validate_string(char* str)
{
  validate_vaddr(str);

  for(; *str!='\0' ; ++str)
  {
    validate_vaddr(str);
  }
}



void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  validate_vaddr(f->esp);
  void* temp_esp = f->esp;

  int sys_call = *(int*) get_next_arg(&temp_esp , sizeof(int));


  switch(sys_call)
  {
    case SYS_HALT:
      sys_halt_wrapper();
      break;

    case SYS_EXIT:
      sys_exit_wrapper(temp_esp);
      break;

    case SYS_EXEC:
      sys_exec_wrapper(temp_esp , &f->eax);
      break;

    case SYS_WAIT:
      sys_wait_wrapper(temp_esp , &f->eax);
      break;

    case SYS_CREATE:
      sys_create_wrapper(temp_esp , &f->eax);
      break;

    case SYS_REMOVE:
      sys_remove_wrapper(temp_esp , &f->eax);
      break;

    case SYS_OPEN:
      sys_open_wrapper(temp_esp , &f->eax);
      break;

    case SYS_FILESIZE:
      sys_filesize_wrapper(temp_esp , &f->eax);
      break;

    case SYS_READ:
      sys_read_wrapper(temp_esp , &f->eax);
      break;

    case SYS_WRITE:
      sys_write_wrapper(temp_esp , &f->eax);
      break;

    case SYS_SEEK:
      sys_seek_wrapper(temp_esp);
      break;

    case SYS_TELL:
      sys_tell_wrapper(temp_esp , &f->eax);
      break;

    case SYS_CLOSE:
      sys_close_wrapper(temp_esp);
      break;

    default:
      sys_exit(-1);
      break;

  }
}


/* System call Wrapper*/
void
sys_halt_wrapper (void)
{
  sys_halt();
}

void
sys_exit_wrapper (void* esp)
{
  int status = *(int*) get_next_arg(&esp , sizeof(int));

  sys_exit(status);
}

void
sys_exec_wrapper (void* esp , uint32_t* eax)
{
  const char* file = *(char**)get_next_arg(&esp , sizeof(char*));

  *eax = sys_exec(file);
}

void
sys_wait_wrapper (void* esp , uint32_t* eax)
{
  pid_t pid = *(pid_t*) get_next_arg(&esp , sizeof(pid_t));

  *eax = sys_wait(pid);
}

void
sys_create_wrapper (void* esp , uint32_t* eax)
{
  const char* file = *(char**)get_next_arg(&esp , sizeof(char*));
  unsigned initial_size = *(unsigned*) get_next_arg(&esp , sizeof(unsigned));

  validate_string((char*)file);

  *eax = sys_create(file , initial_size);
}

void
sys_remove_wrapper (void* esp , uint32_t* eax)
{
  const char* file = *(char**)get_next_arg(&esp , sizeof(char*));

  validate_string((char*)file);

  *eax = sys_remove(file);
}

void
sys_open_wrapper (void* esp , uint32_t* eax)
{
  const char* file = *(char**)get_next_arg(&esp , sizeof(char*));

  validate_string((char*)file);

  *eax = sys_open(file);
}

void
sys_filesize_wrapper (void* esp , uint32_t* eax)
{
  int fd = *(int*) get_next_arg(&esp , sizeof(int));

  *eax = sys_filesize(fd);
}

void
sys_read_wrapper (void* esp , uint32_t* eax)
{
  int fd = *(int*) get_next_arg(&esp , sizeof(int));
  void* buffer = *(void**)get_next_arg(&esp , sizeof(void*));
  unsigned size = *(unsigned*) get_next_arg(&esp , sizeof(unsigned));

  validate_buffer(buffer , size);

  *eax = sys_read(fd , buffer , size);
}

void
sys_write_wrapper (void* esp , uint32_t* eax)
{
  int fd = *(int*) get_next_arg(&esp , sizeof(int));
  const void* buffer = *(void**)get_next_arg(&esp , sizeof(void*));
  unsigned size = *(unsigned*) get_next_arg(&esp , sizeof(unsigned));

  validate_buffer(buffer , size);

  *eax = sys_write(fd , buffer , size);
}

void
sys_seek_wrapper (void* esp)
{
  int fd = *(int*) get_next_arg(&esp , sizeof(int));
  unsigned position = *(unsigned*) get_next_arg(&esp , sizeof(unsigned));

  sys_seek(fd , position);
}

void
sys_tell_wrapper (void* esp , uint32_t* eax)
{
  int fd = *(int*) get_next_arg(&esp , sizeof(int));
  *eax = sys_tell(fd);
}

void
sys_close_wrapper (void* esp)
{
  int fd = *(int*) get_next_arg(&esp , sizeof(int));
  sys_close(fd);
}



/* System call implementations. */
void
sys_halt (void)
{
  shutdown_power_off();
}

void
sys_exit (int status)
{

  struct thread *curr_thread = thread_current();
  curr_thread->exit_status = status;


  struct thread *parent = curr_thread->parent;


    tid_t child_tid = curr_thread->tid;
    struct list_elem *element;
    struct child *child = NULL;

    printf("%s: exit(%d)\n", curr_thread->name , status);
    enum intr_level old_level = intr_disable();
    for (element = list_begin(&parent->children);
		  element != list_end(&parent->children);
		  element = list_next(element))
      {
        struct child *tmp = list_entry(element, struct child, elem);
        if (tmp->self->tid == child_tid)
        {
        child = tmp;
        break;
        }
      }
    intr_set_level(old_level);

    parent->last_child_exit_status = status;

    child->has_exited = true;

    // Release all locks held by the current thread

    old_level = intr_disable();
    element = list_begin(&curr_thread->locks_list);

    while(element != list_end(&curr_thread->locks_list))
    {
        struct list_elem *next_elem = list_next(element);
        struct lock *lock = list_entry(element, struct lock, elem);
        lock_release(lock);

        element = next_elem;
    }

    intr_set_level(old_level);


    sema_up(&parent->sema_wait);









  thread_exit();

}

pid_t
sys_exec (const char *file)
{
  return process_execute(file);
}

int
sys_wait (pid_t pid)
{
  return process_wait(pid);
}

bool
sys_create (const char *file, unsigned initial_size)
{
  if (file == NULL)
    sys_exit(-1);

  lock_acquire(&filesys_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&filesys_lock);

  return success;

}

bool
sys_remove (const char *file)
{
  if (file == NULL)
    sys_exit(-1);

  lock_acquire(&filesys_lock);
  bool success = filesys_remove(file);
  lock_release(&filesys_lock);

  return success;

}

int
sys_open (const char *file)
{
  if (file == NULL)
    return -1;

  lock_acquire(&filesys_lock);
  struct file *f = filesys_open(file);
  lock_release(&filesys_lock);

  if (f == NULL)
    return -1;

  /* Find an unused file descriptor */
  struct thread *t = thread_current();
  int fd;

  for (fd = 2; fd < 128; fd++) {  /* Skip fd 0 and 1 (stdin/stdout) */
    if (t->fd_table[fd] == NULL) {
      t->fd_table[fd] = f;
      return fd;
    }
  }

  /* No available fd */
  file_close(f);
  return -1;

}

int
sys_filesize (int fd)
{
  struct thread *t = thread_current();

  if (fd < 0 || fd >= 128 || t->fd_table[fd] == NULL)
    return -1;

  lock_acquire(&filesys_lock);
  int size = file_length(t->fd_table[fd]);
  lock_release(&filesys_lock);

  return size;

}

int sys_read (int fd, void *buffer, unsigned size)
{
  if (buffer == NULL)
    sys_exit(-1);

  struct thread *t = thread_current();
  int bytes_read = -1;

  /* Handle STDIN */
  if (fd == 0) {
    lock_acquire(&filesys_lock);
    uint8_t *buf = buffer;
    for (unsigned i = 0; i < size; i++)
      buf[i] = input_getc();
    bytes_read = size;
    lock_release(&filesys_lock);
    return bytes_read;
  }

  /* Check for valid fd */
  if (fd < 0 || fd >= 128)
    return -1;

  lock_acquire(&filesys_lock);
  if (t->fd_table[fd] != NULL) {
    bytes_read = file_read(t->fd_table[fd], buffer, size);
  }
  lock_release(&filesys_lock);

  return bytes_read;
}

int sys_write (int fd, const void *buffer, unsigned size)
{
  if (buffer == NULL)
    sys_exit(-1);

  struct thread *t = thread_current();
  int bytes_written = -1;

  /* Handle STDOUT */
  if (fd == 1) {
    lock_acquire(&filesys_lock);
    putbuf(buffer, size);
    bytes_written = size;
    lock_release(&filesys_lock);
    return bytes_written;
  }

  /* Check for valid fd */
  if (fd < 0 || fd >= 128)
    return -1;

  lock_acquire(&filesys_lock);
  if (t->fd_table[fd] != NULL) {
    bytes_written = file_write(t->fd_table[fd], buffer, size);
  }
  lock_release(&filesys_lock);

  return bytes_written;
}

void
sys_seek (int fd, unsigned position)
{
  struct thread *t = thread_current();

  if (fd < 0 || fd >= 128 || t->fd_table[fd] == NULL)
    return;

  lock_acquire(&filesys_lock);
  file_seek(t->fd_table[fd], position);
  lock_release(&filesys_lock);
}

unsigned
sys_tell (int fd)
{
  struct thread *t = thread_current();

  if (fd < 0 || fd >= 128 || t->fd_table[fd] == NULL)
    return -1;

  lock_acquire(&filesys_lock);
  unsigned pos = file_tell(t->fd_table[fd]);
  lock_release(&filesys_lock);

  return pos;
}

void
sys_close (int fd)
{
  struct thread *t = thread_current();

  if (fd < 0 || fd >= 128 || t->fd_table[fd] == NULL)
    return;

  lock_acquire(&filesys_lock);
  file_close(t->fd_table[fd]);
  lock_release(&filesys_lock);

  t->fd_table[fd] = NULL;
}




