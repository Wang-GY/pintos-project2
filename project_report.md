# OS project 2 Report

王戈扬
11510050
陆克渊
11510052

## 结果说明
  本次project我基于助教提供的新的pintos源码，因此project1的特性没有实现，但是可以通过所有project2的测试用例
  ![avatar](http://ww1.sinaimg.cn/large/006lTfYGgy1fsdzx7j7fnj31hc0rwjz0.jpg)

## 感谢
  在我做这个project时戴志豪同学给了我非常大的帮助。提醒了我文件系统的同步性问题，我们也一起讨论了许多我遇到的bug。在这里提出感谢。

## 设计思路

### 综述

本次project需要修改的文件比较集中，各部分功能也十分明确。一个进程开始的时候会执行process_execute和star_process,在这两个方法中我们要实现参数传递。进程退出的时候会直行process_exit，因此我们需要在这里打印输出结果。

### 参数传递
  我将参数传递放在`start_process`中实现，在这个函数中按照x86体系标准处理好`intr_frame`的esp即可实现参数传递。这个函数最后会模拟一个中断，在`intr_frame`中进行的处理就可以得以体现了。

### 系统调用

系统调用sys_call集中在syscall.c中，我们需要自己分别实现system cal之后再通过syscall_init注册之后实现。
系统调用的参数传递靠`struct intr_frame`，我们要从中根据栈指针`esp`解析出需要用的参数，然后检查指针和参数是否合法，如果不合法则会造成进程退出。合法的话进行后续的功能。我实现sys_call的时候每一个系统调用写了两个方法，`sys_*`负责检查参数的合法性，保证文件系统调用的同步性。另外一个函数负责实现功能。这样分工明确，代码可读性好，也便于维护。下面我介绍我比较关键的系统调用的实现。
- process_wait

  这个系统调用是通过一个管道实现的。每一个进程退出的时候向内核的一个管道写入自己的退出信息。父进程调用`process_wait`时，会检查这个管道中是否自己需要的信息，如果有则读取出来并且返回，如果没有则会向管道中写入一个wait请求，然后被一个信号量阻塞。当一个进程退出的时候，它在写入自己的退出信息之后会检查有没有父进程的wait请求，如果有的话就唤醒此进程。
  这种实现方式具有良好的抽象，代码简洁易懂。而且这个管道也会在后面的`process_exec`中发挥作用。管道的具体实现在本部分的最后给出。
  process_wait的实现变得极其简单:
  ```c
  int
  process_wait (tid_t child_tid)
  {
    /*
    check whether child_tid is a child of current thread.
    if so, remove it from children list
    */
    if(!can_wait(child_tid)){
      return -1;
    }
    return read_pipe(child_tid,WAIT)
  }
  ```
- process_exit

  原来的实现没有充分释放资源，在这里我们要释放此进程所有占用资源，*关闭本身的可执行文件*。退出时向管道写入消息.在syscall.c/exit中
  已经关闭了此进程所有的打开的文件.
  ```c
  void
  process_exit (void)
  {
    struct thread *cur = thread_current ();
    uint32_t *pd;

    printf("%s: exit(%d)\n", cur->name, cur->exit_status);
    write_pipe(cur->tid,WAIT,cur->exit_status);
    file_close(cur->executable);

    /* Destroy the current process's page directory and switch back
       to the kernel-only page directory. */
    ...

    }
  ```

- process_exec & start_process

  执行文件并返回子进程id,如果执行失败则返回-1,这里我们也采用了管道通信.因为thread_create一定会返回一个合法的id,
  但子进程创建失败是start_process的异常,所以我们要接受来自start_process的消息.

  ```c
  tid_t
  process_execute (const char *file_name)
  {
  ...
  // create thread
  tid = thread_create (argv[0], PRI_DEFAULT, start_process, fn_copy);
  // read information from pipe
  tid = read_pipe(tid,EXEC);
  ...

  /*
  add tid as child of current process
  */
  p->thread = child->tid;

  list_push_back(&thread_current()->children,&p->elem);
  ...
  return tid;
  }
  ```

  ```c
  /* A thread function that loads a user process and starts it
     running. */
  static void
  start_process (void *file_name_)
  {
    ...
    success = load (argv[0], &if_.eip, &if_.esp);
    if (!success){
      // load fial set exit status
      // free(command_bak);
      write_pipe(thread_current()->tid,EXEC,TID_ERROR);
      exit(-1);
    }
    ...
    //success
    int id = thread_current()->tid;
    write_pipe(id,EXEC,id);
    ...

    /*
    set up arguments passing
    */

    int i=argc;
    char * addr_arr[argc];
    //printf("%s\n","try to put args" );
    //printf("Address\t         Nmae\t        Data\n");
    while(--i>=0){
      if_.esp = if_.esp - sizeof(char)*(strlen(argv[i])+1); //+1: extra \0

      addr_arr[i]=(char *)if_.esp;
      memcpy(if_.esp,argv[i],strlen(argv[i])+1);
      //strlcpy(if_.esp,argv[i],strlen(argv[i])+1);
      //printf("%d\targv[%d][...]\t'%s'\n",if_.esp,i,(char*)if_.esp);

    }

    // 4k  对齐
    //world-align
    while ((int)if_.esp%4!=0) {
      if_.esp--;
    }
    //printf("%d\tworld-align\t0\n", if_.esp);

    i=argc;
    if_.esp = if_.esp-4;
    (*(int *)if_.esp)=0;
    //printf("%d\targv[%d]\t%d\n",if_.esp,i,*((int *)if_.esp));
    while (--i>=0) {

      if_.esp = if_.esp-4;//sizeof()
      (*(char **)if_.esp) = addr_arr[i]; // if_.esp a pointer to uint32_t*
      //printf("%d\targv[%d]\t%d\n",if_.esp,i,(*(char **)if_.esp));
    }

    if_.esp = if_.esp-4;
    (*(char **)if_.esp)=if_.esp+4;
    //printf("%d\targv\t%d\n",if_.esp,(*(char **)if_.esp));

    //put argc
    if_.esp = if_.esp-4;
    (*(int *)if_.esp)=argc;
    //printf("%d\targc\t%d\n",if_.esp,(*(int *)if_.esp));

    //put return address 0
    if_.esp = if_.esp-4;
    (*(int *)if_.esp)=0;


  }
  ```

- file system calls

  在file.c和filesys.c中已经有了基本的实现，我们的设计原则是在保证同步的前提下合理地调度这些资源.当一个进程打开一个文件时,内核会分配给这个进程
  一个文件描述符,(fd_entry).系统维护所有打开文件的列表,进程维护一个所有自己的打开的文件的列表.我们需要保证进程退出时这些资源都被释放.

#### 管道
  管道在我们的实现中扮演了十分重要的角色，这也是我们最喜欢的设计。在这里给出实现。
  process.h
  ```c
  /**
  a pipe to record process's exit status to implement wait sys call
  read list
  write list
  */
  enum action{
    EXEC,
    WAIT
  };
  ```
  process.c
  ```c
  /*
  the list save all elem ready to read
  */
  static struct list read_list;
  /**
  the list to save all read request
  */
  static struct list wait_list;

  struct read_elem{
    int pid;
    enum action action;
    struct list_elem elem;
    int value;
  };

  struct wait_elem{
    int pid;
    enum action action;
    struct list_elem elem;
    struct semaphore sema;
  };


  // called in init.c
  void pipe_init(){
    list_init(&read_list);
    list_init(&wait_list);
  }

  /*
  add an elem to read list
  */
  void write_pipe(int pid,enum action action,int value){
    enum intr_level old_level = intr_disable ();
    // printf("%d write pipe %d, %d, %d\n",thread_tid(),pid, action, value);
    /*
    create a elem in read_list
    */
    struct read_elem* read = malloc(sizeof(struct read_elem));
    read->pid = pid;
    read->action = action;
    read->value = value;
    list_push_back(&read_list,&read->elem);

    /*
    wake up the read request if necessary
    */
    struct list_elem *e;
    for(e=list_begin(&wait_list);e!=list_end(&wait_list);e=list_next(e)){
      struct wait_elem *we = list_entry(e,struct wait_elem,elem);
      if(we->pid == pid && we->action == action)
        sema_up(&we->sema);
    }
    intr_set_level(old_level);
  }



  /*
  read the value in read list.
  create a read request if what the request want is not in read_list yet.
  */
  int read_pipe(int pid,enum action action){
    enum intr_level old_level = intr_disable ();
    // printf("%d read pipe %d, %d\n",thread_tid(),pid, action);
    for(;;){
      /*
      check if what reader want already ready
      */
    struct list_elem *e;
    for(e = list_begin(&read_list); e != list_end(&read_list); e = list_next(e) ){
      struct read_elem *re = list_entry(e,struct read_elem, elem);
      if(re->pid == pid && re->action == action){
        list_remove(e);
        int value = re->value;
        free(re);
        return value;
      }
      intr_set_level(old_level);
    }
    /*
    what reader want is not in read_list, create a wait request
    */
    struct wait_elem *we = malloc(sizeof(struct wait_elem));
    sema_init(&we->sema,0);
    we->pid = pid;
    we->action = action;
    list_push_back(&wait_list,&we->elem);
    sema_down(&we->sema);
    /*
    a writer has write something this reader want, the reader was unblocked and
    clean the request and go to beginning
    */
    list_remove(&we->elem);
    free(we);
  }
  }
  ```

## 数据结构和函数
- thread.h
```c
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    #ifdef USERPROG
        /* Owned by userprog/process.c. */
        tid_t parent_id;                    /* parent pid (tid) */
        uint32_t *pagedir;                  /* Page directory. */
        struct list children;               /* child processes */
        struct list fd_list;       /* List of all file_descriptor it owns*/
        int exit_status;
        struct file *executable;     /* The thread's executable*/
    #endif

        /* Owned by thread.c. */
        unsigned magic;                     /* Detects stack overflow. */
      };
```

在这个任务中，我定义了`struct list children`存储所有的子进程。`struct lsit fd_list`存储所有的文件描述符（所有打开的文件。`int exit_status`记录线程的退出代码

- thread.c
在这个文件中初始化我定义的变量，因此在init_thread作出如下修改
```c
#ifdef USERPROG

  list_init(&(t->fd_list));
  list_init(&t->children);
#endif
```
- process.h

```c
/**
file descriptor
*/
struct file_descriptor{
  int fd;
  struct list_elem elem; // list elem of a process's ownd file descriptor list
  struct file* file; //opend files
};

struct process{
  struct list_elem elem;
  // struct thread *thread;
  int thread; // thread id
};
```

- syscall.c

```c
// called by syscall_handler directly. provid pointer check and ensure synchronization.
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
// each implement sys_call, arguments checked by previous functions.
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
```

syscall.h
```c
// exit process and release all resources, also used by other files, so we define it here.
void exit(int status);
```

函数的功能可以在文档中找到,函数的具体实现就不在报告中给出了.

##  可以改进的地方
1. 如果子进程没有父进程等待(父进程先退出),写入管道的信息就不会被清除,执行很多进程之后就会造成空间浪费,我们尝试释放这部分资源
但是带来了一些问题.
2. 如果父进程在等待的时候被杀死,我们希望清除它所有读的请求.但是父进程在等待的时候还无法退出,因为我们的管道实现功能还比较单一.



## 遇到过的问题
1. 之前实进程文件管理的时候,没有统一在syscall.c中处理,而是在process_exit中释放fd_entry,但是造成page_fault.至今不知道是为什么.于是我重新实现了这一部分
2. 无法通过syn-remove,这是因为在本来应该传入`const char*`的地方不小心传入`char`,编译可以通过但是用例无法通过.
  ```c
  char *file_name = *(char **)(f->esp+4);
  //old:
  // char file_name = *(char **)(f->esp+4);
  f->eax = remove(file_name);
  ```
3. (野指针问题)维护一个进程的子进程是通过threa.children实现的,列表中的元素`struct process`原来记录的是
  子进程的`struct thread* thread`,但是子进程退出之后会释放资源,这个指针会变成野指针,导致无法正确获取id,所有无法通过
  方法`can_wait(tid_t child_tid)`.碰巧除了syn-read, sn-write这两个涉及大量wait的用例才会造成问题.因为其他用例在子
  进程被销毁之前先通知了父进程,父进程还没有接触到野指针.这个问题困扰了起码三天.最终通过更改`struct process`解决.

## 代码风格

我代码风格是尽可能多写方法,多用方法,这样可以避免大量的复制粘贴代码,而且也提高了代码的可读性.我没有自己实现list方法,而是
一直调用原本的数据结构.方法命名采用下划线,意思比较明确,而且也契合pintos源码的命名风格.我的代码量大概在800行左右.我在代码中留有大量注释,方便阅读也记录了自己的思路.

## Hack Testing

### 进程创建攻击
  创建尽可能多的简单进程直到无法继续创建,我们期望至少可以创建60个子进程.(oom仅要求80个).如果内核对空间的利用浪费过大或者忘记释放不必要的资源,将无法通过这个测试.
  ```c
  #include <syscall.h>
  #include "tests/lib.h"
  #include "tests/main.h"
  #define CHILD_CNT 60

  void test_main(void){
    for(int i=0;i < CHILD_CNT; i++){
      exec("child-simple");
    }
    msg("success, execute %d children",CHILD_CNT);
  }
  ```
- 理想结果
``` perl
# -*- perl -*-
use strict;
use warnings;
use tests::tests;

check_expected (IGNORE_EXIT_CODES => 1,IGNORE_USER_FAULTS =>1, [<<'EOF',<<'EOF',<<'EOF']);
(wgy-test-1) begin
(wgy-test-1) success, execute 60 children
(wgy-test-1) end
EOF
(wgy-test-1) begin
(wgy-test-1) success, execute 60 children
(wgy-test-1) end
(child-simple) run
EOF
(wgy-test-1) begin
(wgy-test-1) success, execute 60 children
(wgy-test-1) end
(child-simple) run
(child-simple) run
EOF
pass;
```

- 我的结果
PASS

```
Copying tests/userprog/wgy-test-1 to scratch partition...
Copying tests/userprog/child-simple to scratch partition...
squish-pty bochs -q
========================================================================
                       Bochs x86 Emulator 2.6.7
              Built from SVN snapshot on November 2, 2014
                  Compiled on Apr 14 2018 at 19:22:50
========================================================================
PiLo hda1
Loading............
Kernel command line: -q -f extract run wgy-test-1
Pintos booting with 4,096 kB RAM...
383 pages available in kernel pool.
383 pages available in user pool.
Calibrating timer...  204,600 loops/s.
hda: 5,040 sectors (2 MB), model "BXHD00011", serial "Generic 1234"
hda1: 195 sectors (97 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 186 sectors (93 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'wgy-test-1' into the file system...
Putting 'child-simple' into the file system...
Erasing ustar archive...
Executing 'wgy-test-1':
(wgy-test-1) begin
(wgy-test-1) success, execute 60 children
(wgy-test-1) end
wgy-test-1: exit(0)
(child-simple) run
child-simple: exit(81)
Execution of 'wgy-test-1' complete.
Timer: 2449 ticks
Thread: 88 idle ticks, 320 kernel ticks, 2041 user ticks
hda2 (filesys): 2431 reads, 378 writes
hda3 (scratch): 185 reads, 2 writes
Console: 1029 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off..
```


### 文件资源攻击
  如果关闭文件之后不释放文件描述符在内核占用的资源,则无法通过本测试.
  在我的实现中,一个进程最多可以打开31370个文件.这个测试将不断打开关闭文件重复50000次.如果关闭文件不释放资源,一定在第31370迭代失败.

  ```c
  #include <syscall.h>
  #include "tests/lib.h"
  #include "tests/main.h"
  // 31370 on my computer
  #define PASS_NUM  50000

  void
  test_main (void)
  {

    int fd =0;
    /*get max numner of files it can open*/
    for(int i=0;true;i++){
    fd= open("sample.txt");
      if(fd == -1){
        msg("open fail");
        return;
      }
      close(fd);
      // sucess
      if(i > PASS_NUM){
        break;
      }
    }
    msg("sucess");
  }
  ```
- 理想结果

```perl
# -*- perl -*-
use strict;
use warnings;
use tests::tests;

check_expected ([<<'EOF']);
(wgy-test-2) begin
(wgy-test-2) sucess
(wgy-test-2) end
wgy-test-2: exit(0)
EOF
pass;
```
- 我的结果
PASS
```
Copying tests/userprog/wgy-test-2 to scratch partition...
Copying ../../tests/userprog/sample.txt to scratch partition...
squish-pty bochs -q
========================================================================
                       Bochs x86 Emulator 2.6.7
              Built from SVN snapshot on November 2, 2014
                  Compiled on Apr 14 2018 at 19:22:50
========================================================================
PiLo hda1
Loading............
Kernel command line: -q -f extract run wgy-test-2
Pintos booting with 4,096 kB RAM...
383 pages available in kernel pool.
383 pages available in user pool.
Calibrating timer...  204,600 loops/s.
hda: 5,040 sectors (2 MB), model "BXHD00011", serial "Generic 1234"
hda1: 195 sectors (97 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 97 sectors (48 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'wgy-test-2' into the file system...
Putting 'sample.txt' into the file system...
Erasing ustar archive...
Executing 'wgy-test-2':
(wgy-test-2) begin
(wgy-test-2) sucess
(wgy-test-2) end
wgy-test-2: exit(0)
Execution of 'wgy-test-2' complete.
Timer: 393652 ticks
Thread: 5939 idle ticks, 200 kernel ticks, 387513 user ticks
hda2 (filesys): 200098 reads, 200 writes
hda3 (scratch): 96 reads, 2 writes
Console: 969 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off..

```
## 参考资料

管道设计的理念来自 https://github.com/pindexis/pintos-project2.git
文件系统调用和维护的设计理念来自 https://github.com/yuwumichcn223/pintos.git
我通读了两种实现的代码,对他们的思路也比较了解,自己选择了我认为他们最精彩的地方参考.
