#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "threads/synch.h"

#define STDIN 0
#define STDOUT 1


#define SYS_CALL_NUM 20
static void syscall_handler (struct intr_frame *);

/* Process identifier. */
typedef int pid_t;


void sys_halt(struct intr_frame* f); /* Halt the operating system. */
void sys_exit(struct intr_frame* f); /* Terminate this process. */
void sys_exec(struct intr_frame* f); /* Start another process. */
void sys_wait(struct intr_frame* f); /* Wait for a child process to die. */
void sys_create(struct intr_frame* f); /* Create a file. */
void sys_remove(struct intr_frame* f);/* Create a file. */
void sys_open(struct intr_frame* f); /*Open a file. */
void sys_filesize(struct intr_frame* f);/* Obtain a file's size. */
void sys_read(struct intr_frame* f);  /* Read from a file. */
void sys_write(struct intr_frame* f); /* Write to a file. */
void sys_seek(struct intr_frame* f); /* Change position in a file. */
void sys_tell(struct intr_frame* f); /* Report current position in a file. */
void sys_close(struct intr_frame* f); /* Close a file. */


//Projects 2 and later.
void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
pid_t exec (const char *file);
int wait (pid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

static struct file *find_file_by_fd (int fd);
static struct fd_entry *find_fd_entry_by_fd (int fd);
static int alloc_fid (void);
static struct fd_entry *find_fd_entry_by_fd_in_process (int fd);

//store all syscalls
static void (*syscall_handlers[SYS_CALL_NUM])(struct intr_frame *); // array of all system calls

/*
file descriptors
*/
struct fd_entry{
  int fd;
  struct file *file;
  struct list_elem elem;
  struct list_elem thread_elem;
};

static struct list file_list;

/*
file descriptor id generaor
generate from 2
to all process
*/
static int
alloc_fid (void)
{
  static int fid = 2;
  return fid++;
}

/*
find fd_entry in current's thread fd_list
*/
static struct fd_entry *
find_fd_entry_by_fd_in_process (int fd)
{
  struct fd_entry *ret;
  struct list_elem *l;
  struct thread *t;

  t = thread_current ();

  for (l = list_begin (&t->fd_list); l != list_end (&t->fd_list); l = list_next (l))
    {
      ret = list_entry (l, struct fd_entry, thread_elem);
      if (ret->fd == fd)
        return ret;
    }

  return NULL;
}

/*
find file be fd id
*/
static struct file *
find_file_by_fd (int fd)
{
  struct fd_entry *ret;

  ret = find_fd_entry_by_fd (fd);
  if (!ret)
    return NULL;
  return ret->file;
}


/**
Creates a new file called file initially initial size bytes in size.
Returns true if successful, false otherwise.
Creating a new file does not open it:
opening the new file is a separate operation
which would require a open system call.

@param file: file name
@param
*/
bool create (const char *file, unsigned initial_size){
  //printf("call create %s\n",file);
    /*
    TODO: check of file_name valid
    */
    return filesys_create(file,initial_size);
}

/*
delete the fiile called file.
return true if successful, false otherwise
A file may be removed regardless of whether it is
open or closed. and removing an open file does not
close it
*/
bool remove (const char *file){
  //printf("call remove file %s\n",file);
  return filesys_remove(file);
}

/*
Opens the file called file. Returns a nonnegative integer handle called a “file descriptor” (fd), or -1 if the file could not be opened. File descriptors numbered 0 and 1 are reserved for the console:
fd 0 (STDIN_FILENO) is standard input, fd 1 (STDOUT_FILENO) is standard ouint open (const char *file){
tput.
The open system call will never return either of these file descriptors,
which are valid as system call arguments only as explicitly described below.
Each process has an independent set of file descriptors.
File descriptors are not inherited by child processes.
When a single file is opened more than once,
whether by a single process or different processes, each open returns a new
file descriptor. Different file descriptors for a single file are
closed independently in separate calls to close and they do not share
*/

int open (const char *file){
    //printf("call open file %s\n",file );  // if (get_user(((uint8_t *)esp)+i) == -1){
  //   return false;
  // }
    /*
    TODO: check valid string
    */
    struct file* f = filesys_open(file);
    // open  fail, kill the process
    if(f == NULL){
      //printf("%s\n","open fails");
      return -1;
    }

    // add file descriptor
    struct fd_entry *fde = (struct fd_entry *)malloc(sizeof(struct fd_entry));
    // malloc fails
    if(fde == NULL){
      file_close(f);
      return -1; // open fail
    }
    struct thread *cur = thread_current();
    fde->fd = alloc_fid();
    fde->file = f;
    list_push_back(&cur->fd_list,&fde->thread_elem);
    list_push_back(&file_list,&fde->elem);

    return fde->fd;

}

/*
wait for process with pid
*/
int wait (pid_t pid){
  return process_wait(pid);
}
/*
write buffer to stdout or file
*/
int write (int fd, const void *buffer, unsigned length){
  if(fd==STDOUT){ // stdout
      putbuf((char *) buffer,(size_t)length);
      return (int)length;
  }else{
    struct file *f = find_file_by_fd(fd);
    if(f==NULL){
      exit(-1);
    }
    return (int) file_write(f,buffer,length);

  }
}


/*
exit curret thread with given status
*/
void exit(int status){

  /* Close all the files */
struct thread *t;
struct list_elem *l;

t = thread_current ();
while (!list_empty (&t->fd_list))
  {
    l = list_begin (&t->fd_list);
    close (list_entry (l, struct fd_entry, thread_elem)->fd);
  }

t->exit_status = status;
thread_exit ();
}

/*
Closes file descriptor fd. Exiting or terminating a process
implicitly closes all its open file descriptors,
 as if by calling this function for each one.
*/
void close (int fd){
  struct fd_entry *f = find_fd_entry_by_fd_in_process(fd);

  // close more than once will fail
  if(f == NULL){
    exit(-1);
  }
  file_close (f->file);
  list_remove (&f->elem);
  list_remove (&f->thread_elem);
  free (f);
}

/*
Reads size bytes from the file open as fd into buffer.
Returns the number of bytes actually read (0 at end of file),
or -1 if the file could not be read (due to a condition other than end of file).
 Fd 0 reads from the keyboard using input_getc().
*/
int read (int fd, void *buffer, unsigned length){
  // printf("call read %d\n", fd);
  if(fd==STDIN){
    for(unsigned int i=0;i<length;i++){
      *((char **)buffer)[i] = input_getc();
    }
    return length;
  }else{
    struct file *f = find_file_by_fd(fd);

    if(f == NULL){
      return -1;
    }
    return file_read(f,buffer,length);
  }
}
/*
create  a process execute this file
*/
pid_t exec (const char *file){
  return process_execute(file);
}


void seek (int fd, unsigned position){

  struct file *f = find_file_by_fd(fd);
  if(f == NULL){
    exit(-1);
  }
  file_seek(f,position);
}

int filesize (int fd){

  struct file *f = find_file_by_fd(fd);
  if(f == NULL){
    exit(-1);
  }
  return file_length(f);

}

unsigned tell (int fd){
  struct file *f = find_file_by_fd(fd);
  if(f == NULL){
    exit(-1);
  }
  return file_tell(f);
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  syscall_handlers[SYS_HALT] = &sys_halt;
  syscall_handlers[SYS_EXIT] = &sys_exit;
  syscall_handlers[SYS_WAIT] = &sys_wait;
  syscall_handlers[SYS_CREATE] = &sys_create;
  syscall_handlers[SYS_REMOVE] = &sys_remove;
  syscall_handlers[SYS_OPEN] = &sys_open;
  syscall_handlers[SYS_WRITE] = &sys_write;
  syscall_handlers[SYS_SEEK] = &sys_seek;
  syscall_handlers[SYS_TELL] = &sys_tell;
  syscall_handlers[SYS_CLOSE] =&sys_close;

  syscall_handlers[SYS_READ] = &sys_read;
  syscall_handlers[SYS_EXEC] = &sys_exec;
  syscall_handlers[SYS_FILESIZE] = &sys_filesize;

  lock_init(&file_lock);
  list_init (&file_list);
}


/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */

   //uint8_t unsigned char
   //uaddr is a address
static int
get_user (const uint8_t *uaddr)
{
    //printf("%s\n", "call get user");
  if(!is_user_vaddr((void *)uaddr)){
    return -1;
  }
  if(pagedir_get_page(thread_current()->pagedir,uaddr)==NULL){
    return -1;
  }
  //printf("%s\n","is_user_vaddr" );
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  if(!is_user_vaddr(udst))
    return false;
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

/*
check the following address is valid:
if one of them are not valid, the function will return false
*/
bool is_valid_pointer(void* esp,uint8_t argc){
  uint8_t i = 0;
for (; i < argc; ++i)
{
  // if (get_user(((uint8_t *)esp)+i) == -1){
  //   return false;
  // }
  if((!is_user_vaddr(esp))||(pagedir_get_page(thread_current()->pagedir,esp)==NULL)){
    return false;
  }
}
return true;
}

/*
return true if it is a valid string
*/
bool is_valid_string(void *str){
  //return true;
  int ch=-1;
while((ch=get_user((uint8_t*)str++))!='\0' && ch!=-1);
  if(ch=='\0')
    return true;
  else
    return false;
}


/* Halt the operating system. */
void sys_halt(struct intr_frame* f){
  shutdown();
};

/* Terminate this process. */
void sys_exit(struct intr_frame* f){
  if(!is_valid_pointer(f->esp+4,4)){
    exit(-1);
  }
  int status = *(int *)(f->esp +4);
  exit(status);
};

/* Start another process. */
void sys_exec(struct intr_frame* f){
  // max name char[16]
  if(!is_valid_pointer(f->esp+4,4)||!is_valid_string(*(char **)(f->esp + 4))){
    exit(-1);
  }
  char *file_name = *(char **)(f->esp+4);
  lock_acquire(&file_lock);
  f->eax = exec(file_name);
  lock_release(&file_lock);
};

/* Wait for a child process to die. */
void sys_wait(struct intr_frame* f){
  pid_t pid;
  if(!is_valid_pointer(f->esp+4,4)){
    exit(-1);
  }
  pid = *((int*)f->esp+1);
  f->eax = wait(pid);
};


void sys_create(struct intr_frame* f){
if(!is_valid_pointer(f->esp+4,4)){
  exit(-1);
}
char* file_name = *(char **)(f->esp+4);
if(!is_valid_string(file_name)){
  exit(-1);
}
unsigned size = *(int *)(f->esp+8);
f->eax = create(file_name,size);

}; /* Create a file. */

void sys_remove(struct intr_frame* f){
  if (!is_valid_pointer(f->esp +4, 4) || !is_valid_string(*(char **)(f->esp + 4))){
    exit(-1);
  }
  char *file_name = *(char **)(f->esp+4);
  f->eax = remove(file_name);

};/* Create a file. */
void sys_open(struct intr_frame* f){

  if (!is_valid_pointer(f->esp +4, 4)){
    exit(-1);
  }
  if (!is_valid_string(*(char **)(f->esp + 4))){
    exit(-1);
  }
  char *file_name = *(char **)(f->esp+4);
  lock_acquire(&file_lock);
  f->eax = open(file_name);
  lock_release(&file_lock);


}; /*Open a file. */

void sys_filesize(struct intr_frame* f){
  if (!is_valid_pointer(f->esp +4, 4)){
    exit(-1);
  }
  int fd = *(int *)(f->esp + 4);

  f->eax = filesize(fd);


};/* Obtain a file's size. */
void sys_read(struct intr_frame* f){
  if (!is_valid_pointer(f->esp + 4, 12)){
    exit(-1);
  }
  int fd = *(int *)(f->esp + 4);
  void *buffer = *(char**)(f->esp + 8);
  unsigned size = *(unsigned *)(f->esp + 12);
  if (!is_valid_pointer(buffer, 1) || !is_valid_pointer(buffer + size,1)){
    exit(-1);
  }
  lock_acquire(&file_lock);
  f->eax = read(fd,buffer,size);
  lock_release(&file_lock);

};

/* Read from a file. */
void sys_write(struct intr_frame* f){
  if(!is_valid_pointer(f->esp+4,12)){
    exit(-1);
  }
  int fd = *(int *)(f->esp +4);
  void *buffer = *(char**)(f->esp + 8);
  unsigned size = *(unsigned *)(f->esp + 12);
  if (!is_valid_pointer(buffer, 1) || !is_valid_pointer(buffer + size,1)){
    exit(-1);
}
  lock_acquire(&file_lock);
  f->eax = write(fd,buffer,size);
  lock_release(&file_lock);
  return;
}; /* Write to a file. */

void sys_seek(struct intr_frame* f){
  if (!is_valid_pointer(f->esp +4, 8)){
    exit(-1);
  }
  int fd = *(int *)(f->esp + 4);
  unsigned pos = *(unsigned *)(f->esp + 8);
  seek(fd,pos);
}; /* Change position in a file. */

void sys_tell(struct intr_frame* f){
  if (!is_valid_pointer(f->esp +4, 4)){
    exit(-1);
  }
    int fd = *(int *)(f->esp + 4);
    f->eax = tell(fd);
}; /* Report current position in a file. */

void sys_close(struct intr_frame* f){
  if (!is_valid_pointer(f->esp +4, 4)){
    return exit(-1);
  }
  int fd = *(int *)(f->esp + 4);

  close(fd);

}; /* Close a file. */



// syscall_init put this function as syscall handler
// switch handler by syscall num
static void
syscall_handler (struct intr_frame *f)
{
  //printf ("system call!\n");
  if(!is_valid_pointer(f->esp,4)){
    exit(-1);
    return;
  }
  int syscall_num = * (int *)f->esp;
  //printf("system call number %d\n", syscall_num);
  if(syscall_num<=0||syscall_num>=SYS_CALL_NUM){
    exit(-1);
  }
  // printf("sys call number: %d\n",syscall_num );
  syscall_handlers[syscall_num](f);

}


static struct fd_entry *
find_fd_entry_by_fd (int fd)
{
  struct fd_entry *ret;
  struct list_elem *l;

  for (l = list_begin (&file_list); l != list_end (&file_list); l = list_next (l))
    {
      ret = list_entry (l, struct fd_entry, elem);
      if (ret->fd == fd)
        return ret;
    }

  return NULL;
}
