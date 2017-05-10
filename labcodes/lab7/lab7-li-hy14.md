## lab7 实验报告

#### 计45 李昊阳 2014011421

### 练习0：填写已有实验

**本实验依赖实验1/2/3/4/5/6。请把你做的实验1/2/3/4/5/6的代码填入本实验中代码中有“LAB1”/“LAB2”/“LAB3”/“LAB4”/“LAB5”/“LAB6”的注释相应部分。并确保编译通过。注意：为了能够正确执行lab7的测试应用程序，可能需对已完成的实验1/2/3/4/5/6的代码进行进一步改进。** 

答： 这里我使用了图形化的比较/merge工具meld来手动合并。


### 练习1：理解内核级信号量的实现和基于内核级信号量的哲学家就餐问题（不需要编码）

#### 问题1.1 给出内核级信号量的设计描述，并说其大致执行流流程。

答：
内核级信号量定义在`kern/sync/sem.h`中,它对外表现为一个带有资源属性和资源修改原子操作的整体，对内表现为一个由整型变量和等待队列构成的结构体，以及基于结构体的一系列操作函数。

```c
typedef struct {
    int value;
    wait_queue_t wait_queue;
} semaphore_t;
```

其中`value`用于表示信号量中资源的整数值，`wait_queue`为等待队列。对于信号量存在以下几个函数操作：
- void sem_init(semaphore_t *sem, int value)：完成信号量的初始化工作，设置value值并创建一个等待队列用来保存等待信号量的进程。
- void up(semaphore_t *sem)：即V操作，调用__up()函数实现。
- void down(semaphore_t *sem)：即P操作，调用__down()函数实现。
- bool try_down(semaphore_t *sem)：非阻塞的P操作，如果信号量的value大于0，则直接减一。

信号量的核心是P操作和V操作，下面具体描述一下其大概执行过程：
1. P操作：具体的实现函数是`__down()`函数，执行流程为：
	- 关中断。
	- 判断当前信号量的`value`是否大于0。
	- 如果`value>0`，则表明可以获得信号量，从而让`value`减一，并开中断返回即可。
	- 如果`value<=0`，则表明无法获得信号量，所以需要将当前的进程加入到等待队列中，然后开中断，运行调度器选择另外一个进程执行。如果被V操作唤醒，则把自身关联的wait从等待队列中删除。

2. V操作：具体的实现函数是`__up()`函数，执行流程为：
	- 关中断。
	- 判断等待队列是否为空。
	- 如果等待队列为空则表示`wait_queue`中没有进程正在等待，所以直接把`value`加一，然后开中断返回。
	- 如果等待队列不为空则表示有进程在等待，则调用`wakeup_wait`函数将`wait_queue`中等待的第一个wait删除并把此wait关联的进程唤醒，然后开中断返回。

就哲学家问题就餐问题而言，`ucore`中基于信号量的哲学家问题以哲学家作为信号量，然后把一个哲学家周围的两把叉子作为一个整体来考虑。要么获取两把叉子开始进餐（进程执行），要不就不能开始吃饭（进程挂起等待）。

首先创建了五个哲学家进程并把五个哲学家信号量初始化为0，即将它作为一个同步事件来处理。哲学家首先进入`THINKING`状态;休眠一段时间后进入临界区，尝试获取左右两边的叉子。如果能获取到就对自身的信号量作UP操作（`UP(me)`），表示同步条件已经完成；如果不能获取到就退出，然后因为同步事件未完成而被down挂起。进餐完毕以后释放自身的两把叉子，并检测两边的哲学家是否能够进餐（`test(i-1)`,`test(i+1)`）。


#### 问题1.2 给出给用户态进程/线程提供信号量机制的设计方案，并比较说明给内核级提供信号量机制的异同。

答：

用户态进程/线程的信号量机制和内核级的信号量机制大体相同，同样都有关键的P操作和V操作，但是由于实现信号量机制包含了开关中断等需要内核才能执行的操作，所以在用户态无法直接执行，因此需要系统调用来完成用户态的信号量机制，可以增加与信号量相关的系统调用，比如SYS_SEMINIT,SYS_UP,SYS_DOWN等。

另外在内核态，所有线程的内存空间都是相同的，故对于一个信号量的访问和操作在语义和实际上都是针对同一个信号量。但是在用户态，不同的用户进程的虚拟内存相互之间是不共享的，因此如果在`fork`时将信号量的这部分内存也同样复制的话，就时区信号量的意义的了，所以使用信号量进行同步互斥的进程之间，除了本身需要共享的一部分内存外，信号量的内存也需要共享。

所以，信号量在用户态和内核态的相同点是实现机制大体相同，不同点主要是权限问题（是否需要系统调用）和是否需要让进程/线程共享一块信号量的内存空间。

### 练习2：完成内核级条件变量和基于内核级条件变量的哲学家就餐问题（需要编码）

#### 问题2.1 给出内核级条件变量的设计描述，并说其大致执行流流程。

ucore中管程数据结构monitor_t的定义在`kern/sync/moniter.h`中：

```c
typedef struct monitor{
    semaphore_t mutex;      // the mutex lock for going into the routines in monitor, should be initialized to 1
    semaphore_t next;       // the next semaphore is used to down the signaling proc itself, and the other OR wakeuped waiting proc should wake up the sleeped signaling proc.
    int next_count;         // the number of of sleeped signaling proc
    condvar_t *cv;          // the condvars in monitor
} monitor_t;
```

其中各个变量的具体含义是：
- mutex: 二值信号量，保证了每次只能有一个进程进入管程，即确保了互斥访问性质。
- cv: 管程中的条件变量cv通过执行`wait_cv`，使得等待某个条件C为真的进程离开管程并进入睡眠状态，进而让其他进程进入管程执行。进入管程的进程设置条件C为真并执行`signal_cv`时，让等待某个条件C为真的睡眠进程被唤醒，继续进入管程中执行。
- next/next_count: 这两个变量是为了配合进程对条件变量cv的操作而设置的，由于进程A在发出`signal_cv`后会唤醒睡眠进程B，进程B转入执行状态又会导致进程A睡眠，直到进程B离开管程，进程A才能继续执行，这个同步过程是通过信号量next完成的；而next_count表示了由于发出`singal_cv`而睡眠的进程个数。


ucore中条件变量的数据结构condvar_t定义在`kern/sync/moniter.h`中：

```c
typedef struct condvar{
    semaphore_t sem;        // the sem semaphore  is used to down the waiting proc, and the signaling proc should up the waiting proc
    int count;              // the number of waiters on condvar
    monitor_t * owner;      // the owner(monitor) of this condvar
} condvar_t;
```

其中各个变量的具体含义是：

- sem: 用于让发出`wait_cv`操作的等待某个条件C为真的进程进入睡眠状态和让发出`signal_cv`操作的进程通过这个sem来唤醒睡眠的进程。
- count: 表示因为等待这个条件变量而睡眠的进程的个数。
- owner: 表示此条件变量属于哪个管程。

针对条件变量，有两个基本的操作`cond_wait`和`cond_signal`，前者是等待条件成立，后者是确认条件成立并唤醒那些等待这个条件成立的进程。

```c
// Suspend calling thread on a condition variable waiting for condition Atomically unlocks 
// mutex and suspends calling thread on conditional variable after waking up locks mutex. Notice: mp is mutex semaphore for monitor's procedures
void
cond_wait (condvar_t *cvp) {
    //LAB7 EXERCISE1: YOUR CODE
    cprintf("cond_wait begin:  cvp %x, cvp->count %d, cvp->owner->next_count %d\n", cvp, cvp->count, cvp->owner->next_count);
   /*
    *         cv.count ++;
    *         if(mt.next_count>0)
    *            signal(mt.next)
    *         else
    *            signal(mt.mutex);
    *         wait(cv.sem);
    *         cv.count --;
    */
    cvp->count++;
    if (cvp->owner->next_count > 0) {
      up(&(cvp->owner->next));
    }
    else {
      up(&(cvp->owner->mutex));
    }
    down(&(cvp->sem));
    cvp->count--;
    cprintf("cond_wait end:  cvp %x, cvp->count %d, cvp->owner->next_count %d\n", cvp, cvp->count, cvp->owner->next_count);
}
```

`cond_wait`的执行流程大体如下：
- 条件变量的count加一。
- 如果`monitor.next_count>0`，表示有进程执行`cond_signal`函数而转入睡眠状态，这些进程构成了一个链表，需要唤醒链表中的一个进程。如果`monitor.next_count<=0`，则需要唤醒由于互斥而不能进入管程的进程链表中的一个进程。
- 对条件变量的信号量执行P操作，请求访问资源。
- 条件变量的count减一。


```c
// Unlock one of threads waiting on the condition variable. 
void 
cond_signal (condvar_t *cvp) {
   //LAB7 EXERCISE1: YOUR CODE
   cprintf("cond_signal begin: cvp %x, cvp->count %d, cvp->owner->next_count %d\n", cvp, cvp->count, cvp->owner->next_count);  
  /*
   *      cond_signal(cv) {
   *          if(cv.count>0) {
   *             mt.next_count ++;
   *             signal(cv.sem);
   *             wait(mt.next);
   *             mt.next_count--;
   *          }
   *       }
   */
  if (cvp->count > 0) {
    cvp->owner->next_count++;
    up(&(cvp->sem));
    down(&(cvp->owner->next));
    cvp->owner->next_count--;
  }
   cprintf("cond_signal end: cvp %x, cvp->count %d, cvp->owner->next_count %d\n", cvp, cvp->count, cvp->owner->next_count);
}
```

`cond_signal`的执行流程大体如下：
- 判断是否存在等待此条件的进程。
- 如果有则将此进程睡眠在`cvp->owner->next`上，等待其他进程将本进程再次唤醒。

#### 问题2.2 给出给用户态进程/线程提供条件变量机制的设计方案，并比较说明给内核级提供条件变量机制的异同。

答：条件变量是基于信号量来实现的，底层的操作最终都会落实到信号量的P操作和V操作上，条件变量可以看成是在前面提到的信号量的实现基础上多了一层封装，当然由于权限问题，用户态进程/线程提供条件变量的一些操作同样需要封装为系统调用接口。另外，用户态进程/线程提供条件变量在创建的时候需要共享内存，这点也是和内核级提供条件变量实现上的区别。

#### 问题2.3 能否不用基于信号量机制来完成条件变量？如果不能，请给出理由，如果能，请给出设计说明和具体实现。

答：不用锁无法保证满足条件后的处理过程中条件一直时满足的, 所以不用信号量可以实现, 但是至少要用某种局部的锁机制(不能用类似于windows中raise irql这样全局性的锁)。

### 说明你的实现与参考答案的区别

和参考答案的实现大体相同。

### 列出你认为本实验中重要的知识点，以及与对应的OS原理中的知识点，并简要说明你对二者的含义，关系，差异等方面的理解（也可能出现实验中的知识点没有对应的原理知识点）

1. 信号量的基本原理与实现

	信号量是一种同步互斥机制的实现，普遍存在于现在的各种操作系统内核里。相比于spinlock，信号量的应用对象是在临界区中运行的时间较长的进程。等待信号量的进程需要睡眠来减少占用 CPU 的开销，当多个（>1）进程可以进行互斥或同步合作时，一个进程会由于无法满足信号量设置的某条件而在某一位置停止，直到它接收到一个特定的信号（表明条件满足了）。为了发信号，需要使用一个称作信号量的特殊变量。为通过信号量s传送信号，信号量的V操作采用进程可执行原语semSignal(s)；为通过信号量s接收信号，信号量的P操作采用进程可执行原语semWait(s)；如果相应的信号仍然没有发送，则进程被阻塞或睡眠，直到发送完为止

2. 条件变量与管程

	引入了管程是为了将对共享资源的所有访问及其所需要的同步操作集中并封装起来。一个管程定义了一个数据结构和能为并发进程所执行（在该数据结构上）的一组操作，这组操作能同步进程和改变管程中的数据。局限在管程中的数据结构，只能被局限在管程的操作过程所访问，任何管程之外的操作过程都不能访问它；另一方面，局限在管程中的操作过程也主要访问管程内的数据结构。由此可见，管程相当于一个隔离区，它把共享变量和对它进行操作的若干个过程围了起来，所有进程要访问临界资源时，都必须经过管程才能进入，而管程每次只允许一个进程进入管程，从而需要确保进程之间互斥。

3. 哲学家就餐问题的模拟方法

### 列出你认为OS原理中很重要，但在实验中没有对应上的知识点

Mesa管程，Hoare管程，Hansen管程的实现和区别等。

