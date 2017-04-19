## lab4 实验报告

#### 计45 李昊阳 2014011421

### 练习0：填写已有实验

**本实验依赖实验1/2/3。请把你做的实验1/2/3的代码填入本实验中代码中有“LAB1”,“LAB2”,“LAB3”的注释相应部分。** 

答： 这里我使用了图形化的比较/merge工具meld来手动合并。


### 练习1：分配并初始化一个进程控制块（需要编码）

在`kern/process/proc.c`中修改`alloc_proc`函数：
```c
// alloc_proc - alloc a proc_struct and init all fields of proc_struct
static struct proc_struct *
alloc_proc(void) {
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL) {
    //LAB4:EXERCISE1 2014011421
    /*
     * below fields in proc_struct need to be initialized
     *       enum proc_state state;                      // Process state
     *       int pid;                                    // Process ID
     *       int runs;                                   // the running times of Proces
     *       uintptr_t kstack;                           // Process kernel stack
     *       volatile bool need_resched;                 // bool value: need to be rescheduled to release CPU?
     *       struct proc_struct *parent;                 // the parent process
     *       struct mm_struct *mm;                       // Process's memory management field
     *       struct context context;                     // Switch here to run process
     *       struct trapframe *tf;                       // Trap frame for current interrupt
     *       uintptr_t cr3;                              // CR3 register: the base addr of Page Directroy Table(PDT)
     *       uint32_t flags;                             // Process flag
     *       char name[PROC_NAME_LEN + 1];               // Process name
     */
        proc->state = PROC_UNINIT;
        proc->pid = -1;
        proc->runs = 0;
        proc->kstack = 0;
        proc->need_resched = 0;
        proc->parent = NULL;
        proc->mm = NULL;
        memset(&(proc->context), 0, sizeof(struct context));
        proc->tf = NULL;
        proc->cr3 = boot_cr3;
        proc->flags = 0;
        memset(proc->name, 0, PROC_NAME_LEN);
    }
    return proc;
}
```

简要说明设计实现过程:
- `alloc_proc`函数完成了`proc_struct`这个基本的进程控制块的初始化工作；
- 对`proc_struct`中的每一个成员进行初始化操作;
	- 一般是赋值为0或者空指针;
	- 个别特殊的域需要特别处理（pid=-1,cr3=boot_cr3）。

#### 与参考答案的不同

和参考答案基本相同。

#### 问题1.1：请说明proc_struct中struct context context和struct trapframe *tf成员变量含义和在本实验中的作用是啥？（提示通过看代码和编程调试可以判断出来）

答：

`context`定义在`kern/process/proc.h`中

```c
struct context {
    uint32_t eip;
    uint32_t esp;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
};
```
**成员变量含义：**上下文切换的过程中，各个寄存器的值；

**作用：** 在进行上下文切换（中断服务例程或异常或系统调用例程）的过程中，使用`context`来保存寄存器的值。具体来说：其保存的是在`proc_run`函数中执行的现场信息，里面有一个语句是`switch_to(&(prev->context), &(next->context));`，这说明`context`对于进程是透明的，进程认为自己进入了服务例程后就直接恢复了，但在其中进程可能被调度过，在进行调度时就需要保存`context`。

`trapframe`是异常帧，定义在`kern/trap/trap.h`中

```c
struct trapframe {
    struct pushregs tf_regs;
    uint16_t tf_gs;
    uint16_t tf_padding0;
    uint16_t tf_fs;
    uint16_t tf_padding1;
    uint16_t tf_es;
    uint16_t tf_padding2;
    uint16_t tf_ds;
    uint16_t tf_padding3;
    uint32_t tf_trapno;
    /* below here defined by x86 hardware */
    uint32_t tf_err;
    uintptr_t tf_eip;
    uint16_t tf_cs;
    uint16_t tf_padding4;
    uint32_t tf_eflags;
    /* below here only when crossing rings, such as from user to kernel */
    uintptr_t tf_esp;
    uint16_t tf_ss;
    uint16_t tf_padding5;
} __attribute__((packed));
```

**成员变量含义：** 在内核堆栈上的进程的上下文信息；

**作用：** 在中断、异常或者系统调用的时候，会由硬件和软件协同来保存在内核堆栈上的进程上下文信息。一部分是由硬件直接保存（还会根据中断时所在的特权级选择性保存ss和esp），另一部分是通过软件来保存的（主要为段寄存器和通用寄存器）。具体来说，在本实验中`trapframe`用来模拟线程的执行环境，在创建完线程并执行这个线程时，需要通过中断返回，`trapframe`的正确与否决定了新建的线程能否正确地执行。


### 练习2：为新创建的内核线程分配资源（需要编码）

在`kern/process/proc.c`中完成`do_fork()`函数：

```c
int
do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf) {
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    if (nr_process >= MAX_PROCESS) {
        goto fork_out;
    }
    ret = -E_NO_MEM;

    //LAB4:EXERCISE2 2014011421
    if ((proc = alloc_proc()) == NULL) {
        goto fork_out;
    }

    proc->parent = current;

    if (setup_kstack(proc) != 0) {
        goto bad_fork_cleanup_proc;
    }
    if (copy_mm(clone_flags, proc) != 0) {
        goto bad_fork_cleanup_kstack;
    }
    copy_thread(proc, stack, tf);

    bool intr_flag;
    local_intr_save(intr_flag);
    {
        proc->pid = get_pid();
        hash_proc(proc);
        list_add(&proc_list, &(proc->list_link));
        nr_process ++;
    }
    local_intr_restore(intr_flag);

    wakeup_proc(proc);

    ret = proc->pid;
fork_out:
    return ret;

bad_fork_cleanup_kstack:
    put_kstack(proc);
bad_fork_cleanup_proc:
    kfree(proc);
    goto fork_out;
}
```

简要说明设计实现过程:
- 调用`alloc_proc`，首先获得一块用户信息块（判断是否出错）；
- 调用`setup_kstack`为进程分配一个内核栈（判断是否出错）；
- 调用`copy_mm`复制原进程的内存管理信息到新进程（判断是否出错）；
- 调用`copy_thread`复制原进程上下文到新进程；
- 将新进程添加到进程列表；
- 调用`wakeup_proc`唤醒新进程；
- 返回新进程号；

#### 与参考答案的不同

与参考答案大体相同，不过答案的实现加入了一部分原子操作，即在将新进程加入进程队列中的前后有关中断和开中断的操作，在比较了之后我认为答案上的实现更好一些就补充了进来。

#### 问题2.1 请说明ucore是否做到给每个新fork的线程一个唯一的id？请说明你的分析和理由。

答：
ucore做到了给每个新fork的线程一个唯一的id。在`do_fork`函数中，通过`get_pid`函数为新进程分配了一个`pid`。在`get_pid`函数中，通过遍历进程链表，找到了一个唯一的`pid`并返回。


### 练习3：阅读代码，理解 proc_run 函数和它调用的函数如何完成进程切换的。（无编码工作）

在`kern/process/proc.c`中的`proc_run`函数：

```c
void
proc_run(struct proc_struct *proc) {
    if (proc != current) {
        bool intr_flag;
        struct proc_struct *prev = current, *next = proc;
        local_intr_save(intr_flag);
        {
            current = proc;
            load_esp0(next->kstack + KSTACKSIZE);
            lcr3(next->cr3);
            switch_to(&(prev->context), &(next->context));
        }
        local_intr_restore(intr_flag);
    }
}
```

`proc_run`函数的执行过程大概是：
- 判断需要切换的进程是否是当前进程；
- 关中断；
- 设置current为需要运行的进程；
- 切换栈；
- 设置CR3寄存器；
- 保存恢复通用寄存器；
- 开中断；

具体来说，`proc_run`函数完成了两个进程的上下文切换，这个切换包括当前进程的控制块的切换、进程页表和内核堆栈的切换以及进程的通用寄存器的保存、恢复。因为其中涉及到寄存器的恢复，所以这个过程应该是原子操作，故屏蔽了一般的中断。首先切换了当前进程的控制块，然后切换了新进程的内核堆栈，之后加载了新的页表，最后进行上下文的切换。

#### 问题3.1 在本实验的执行过程中，创建且运行了几个内核线程？

答：2个。第1个是idle线程，第2个是init线程。

#### 问题3.2 语句local_intr_save(intr_flag);....local_intr_restore(intr_flag);在这里有何作用?请说明理由

答：作用是屏蔽中断，用于关中断和开中断。因为进程上下文切换时有通用寄存器的保存和恢复，所以需要让切换进程的过程不被中断打断。

### 列出你认为本实验中重要的知识点，以及与对应的OS原理中的知识点，并简要说明你对二者的含义，关系，差异等方面的理解（也可能出现实验中的知识点没有对应的原理知识点）

- 内核线程创建/执行的管理过程，与原理课上相应的理论知识相对应。。
- 内核线程的切换和基本调度过程，与原理课上相应的理论知识相对应。
- 进程的属性。进程是指一个具有一定独立功能的程序在一个数据集合上的一次动态执行过程。我们可以从四个方面理解进程：程序、数据集合、执行、动态执行过程。程序是一段特定的指令机器码序列，CPU一条一条地取出在内存中程序的指令并按照指令的含义执行各种功能。数据集合是使用的内存。执行是让CPU工作，数据集合和执行共同体现了进程对资源的占用。动态执行过程体现了程序执行的不同“生命”阶段。具体对应到实验中，ucore需要设计一个数据结构来表示进程（或者线程）的这些所有的属性，即进程控制块proc_struct,控制块中包含了进程的各个属性。

### 列出你认为OS原理中很重要，但在实验中没有对应上的知识点

- `fork()`函数的具体展开等；
- 进程队列等知识；


