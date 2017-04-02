## lab3实验报告

#### 计45 李昊阳 2014011421

### 练习0：填写已有实验

**本实验依赖实验1/2。请把你做的实验1/2的代码填入本实验中代码中有“LAB1”,“LAB2”的注释相应部分。** 

答： 这里我使用了图形化的比较/merge工具meld来手动合并。


### 练习1：给未被映射的地址映射上物理页（需要编程）

在`kern/mm/vmm.c`中补全`do_pgfault`函数，根据注释添加下面的代码即可实现给未被映射的地址映射上物理页：

```c
    ptep = get_pte(mm->pgdir, addr, 1);
    
    if (ptep == NULL) {
        cprintf("get_pte in do_pgfault failed\n");
        goto failed;
    }
    
    if (*ptep == 0) { // if the phy addr isn't exist, then alloc a page & map the phy addr with logical addr
        if (pgdir_alloc_page(mm->pgdir, addr, perm) == NULL) {
            cprintf("pgdir_alloc_page in do_pgfault failed\n");
            goto failed;
        }
    }
    else { // if this pte is a swap entry, then load data from disk to a page with phy addr
           // and call page_insert to map the phy addr with logical addr
        if(swap_init_ok) {
            struct Page *page=NULL;
            if ((ret = swap_in(mm, addr, &page)) != 0) {
                cprintf("swap_in in do_pgfault failed\n");
                goto failed;
            }    
            page_insert(mm->pgdir, page, addr, perm);
            swap_map_swappable(mm, addr, page, 1);
            page->pra_vaddr = addr;
        }
        else {
            cprintf("no swap_init_ok but ptep is %x, failed\n",*ptep);
            goto failed;
        }
   }
   ret = 0;
failed:
    return ret;
```

简要说明设计实现过程:
- 通过虚拟地址获得pte；
- 判断pte是否为空，如果为空则调用`pgdir_alloc_page()`分配相应的物理页以建立映射关系；
- **（下面的这几步是实现页面替换算法，严格来说属于练习2中应该完成的部分）**
- 如果pte不为空，那么说明该页在外存中，之后应该换入；
- 从外存中替换入相应的页；
- 更新页表的映射关系；
- 设置页可交换；
- 更新页的虚拟地址；

#### 与参考答案的不同

答案中检查了`get_pte`失败的情况并给出了相应的提示信息，我的实现中没有考虑这一点。虽然这并不影响本次实验的正确性，但还是要好一些，所以就补充了进来。

#### 问题1.1：请描述页目录项（Pag Director Entry）和页表（Page Table Entry）中组成部分对ucore实现页替换算法的潜在用处。

答：
页目录项的结构为：
```
31           11
+------------+--------------+-+---+-+-+---+---+---+---+-+
|PT19 ... PT0|Reserved(3bit)|0|PSE|0|A|PCD|PWT|U/S|W/R|P|
+------------+--------------+-+---+-+-+---+---+---+---+-+
```
其中高20位表示页表对应物理地址的高20位，最低位P表示是否存在对应页表，W/R表示可读可写性质，U/S表示权限，A表示是否被访问

页表的结构为：
```
31           11
+--------------+--------------+-+---+-+---+---+---+---+-+
|PFN19 ... PFN0|Reserved(3bit)|0|D|0|A|PCD|PWT|U/S|W/R|P|
+--------------+--------------+-+---+-+---+---+---+---+-+
```
其中高20位表示对应物理页的页号，即物理页的最高二十位。控制位和页目录项类似，但是增加了一个新的标志位D表示该页是否被修改。控制位对ucore很有用，A和D分别表示访问位和修改位，在实现原理课上讲的时钟算法和改进的时钟算法时，会用到这两个标志位。

总之，PDE和PTE对ucore实现页替换算法的潜在用处可以概括为：提供虚拟地址到物理地址的转换，用于判断相应的物理地址是否合法、物理页是否在内存中以及记录访问权限和相关信息等。

#### 问题1.2：如果ucore的缺页服务例程在执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？

答：如果缺页服务例程在执行过程中发生了页访问异常，硬件首先会保存现场上下文信息，然后禁止嵌套中断。由于一般在处理缺页异常时是由缺页服务例程来处理，但此时由于缺页服务例程也缺页，故需要事先约定好服务例程的对换区地址，由硬件将其加载进内存。


### 练习2：补充完成基于FIFO的页面替换算法（需要编程）

在`kern/mm/vmm.c`中补全`do_pgfault`函数，其中下面这部分是实现页面替换算法的部分：

```c
        if(swap_init_ok) {
            struct Page *page=NULL;
            if ((ret = swap_in(mm, addr, &page)) != 0) {
                cprintf("swap_in in do_pgfault failed\n");
                goto failed;
            }    
            page_insert(mm->pgdir, page, addr, perm);
            swap_map_swappable(mm, addr, page, 1);
            page->pra_vaddr = addr;
        }
        else {
            cprintf("no swap_init_ok but ptep is %x, failed\n",*ptep);
            goto failed;
        }
```

这部分代码是在判断出`*ptep != 0`后执行的。由于pte不为空,说明该页在外存中，之后应该换入。所以上述代码的设计实现过程是:

- 从外存中替换入相应的页；
- 更新页表的映射关系；
- 设置页可交换；
- 更新页的虚拟地址；

在真正执行基于FIFO的页面替换算法时，需要在`kern/mm/swap_fifo.c`中，修改`_fifo_map_swappable()`函数和`_fifo_swap_out_victim()`函数：

```c
/*
 * (3)_fifo_map_swappable: According FIFO PRA, we should link the most recent arrival page at the back of pra_list_head qeueue
 */
static int
_fifo_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    list_entry_t *entry=&(page->pra_page_link);
 
    assert(entry != NULL && head != NULL);
    //record the page access situlation
    /*LAB3 EXERCISE 2: 2014011421*/ 
    //(1)link the most recent arrival page at the back of the pra_list_head qeueue.
    list_add(head, entry);
    return 0;
}
/*
 *  (4)_fifo_swap_out_victim: According FIFO PRA, we should unlink the  earliest arrival page in front of pra_list_head qeueue,
 *                            then set the addr of addr of this page to ptr_page.
 */
static int
_fifo_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
{
     list_entry_t *head=(list_entry_t*) mm->sm_priv;
         assert(head != NULL);
     assert(in_tick==0);
     /* Select the victim */
     /*LAB3 EXERCISE 2: 2014011421*/ 
     //(1)  unlink the  earliest arrival page in front of pra_list_head qeueue
     //(2)  set the addr of addr of this page to ptr_page
     /* Select the tail */
     list_entry_t *le = head->prev;
     assert(head!=le);
     struct Page *p = le2page(le, pra_page_link);
     list_del(le);
     assert(p !=NULL);
     *ptr_page = p;
     return 0;
}
```

基于FIFO的页面替换算法只考虑页面第一次被访问的时间，因此我可以将ucore访问的物理页按照分配的顺序组织成一条链表，其中头部为最先访问的页，尾部为最后访问的页；在需要换出页面时，将表头的页面换出即可。具体的设计实现过程是：

- 当有新访问的页面时，将其加入环形链表的尾部；
- 当需要换出页面时，调用`list_del`将头部的页删除；

#### 与参考答案的不同
答案中进行了较为充分的assert检查，这虽然不影响实验的正确性但却有利于程序的调试，所以我之后也补充了进来。


#### 问题2.1 如果要在ucore上实现"extended clock页替换算法"请给你的设计方案，现有的swap_manager框架是否足以支持在ucore中实现此算法？如果是，请给你的设计方案。如果不是，请给出你的新的扩展和基此扩展的设计方案。并需要回答如下问题。
- 需要被换出的页的特征是什么？
- 在ucore中如何判断具有这样特征的页？
- 何时进行换入和换出操作？

答：
现有的框架足以支持在ucore中实现extended clock页替换算法。

设计方案：
改进时钟算法要求所有页面组织成一个环形的队列，这一点可以通过`list_entry`实现；另外由于页表项中记录了访问位和修改位，因此改进时钟算法的必要条件均已具备，如下图所示（A为访问位，D为修改位）。
```
31           11
+--------------+--------------+-+---+-+---+---+---+---+-+
|PFN19 ... PFN0|Reserved(3bit)|0|D|0|A|PCD|PWT|U/S|W/R|P|
+--------------+--------------+-+---+-+---+---+---+---+-+
```
但是我们同样注意到，现有的框架中没有动态修改访问页面的函数，所以我们需要在现有的框架中加入访问某页时的函数，以实现动态地修改页面的访问位和修改位。

- 需要被换出的页的特征是什么？
	- 答：访问位和修改位都为0。（这可能是通过多轮修改后的结果）
- 在ucore中如何判断具有这样特征的页？
	- 答：通过页表项的标志位可以判断页面是否被访问以及是否被修改。
- 何时进行换入和换出操作？
	- 答：当发生缺页异常且判断所需页面在外存中，则需要换入；如果内存没有空闲页，则要进行扫描，如果遇到A和D位都为0的页面，那么就执行换出。

### 列出你认为本实验中重要的知识点，以及与对应的OS原理中的知识点，并简要说明你对二者的含义，关系，差异等方面的理解。

1. 中断处理与缺页异常
	- 本次实验涉及了中断处理，与lab1初始化中断管理和简单地处理时钟中断不同的是，这次需要更为复杂的中断处理——缺页异常。CPU把引起缺页异常的虚拟地址装到CR2寄存器中，然后给出了出错码（tf->tf_err），指示引起缺页异常的存储器访问的类型。中断服务例程会调用缺页异常处理函数`do_pgfault`进行处理。
	- ucore中`do_pgfault`函数是完成缺页异常处理的主要函数，它根据从CPU的控制寄存器CR2中获取的缺页异常的虚拟地址以及根据error code的错误类型来查找此虚拟地址是否在某个VMA的地址范围内以及是否满足正确的读写权限，如果在此范围内并且权限也正确，这认为这是一次合法访问，但没有建立虚实对应关系。所以需要分配一个空闲的内存页，并修改页表完成虚地址到物理地址的映射，刷新TLB，然后调用iret中断，返回到产生缺页异常的指令处重新执行此指令。如果该虚地址不再某VMA范围内，这认为是一次非法访问。
	- 实验的这一部分与原理课上所讲的缺页异常处理过程相对应。

2. 页面替换算法
	- os在原理课上，老师讲了众多页面算法，比如最优替换算法、LRU替换算法、时钟替换算法、FIFO替换算法等。
	- 在实验中，我们实现了FIFO算法框架：构建了一个链表，表头为最先访问的页面，每次需要换出时从链表头部考虑。不过基于FIFO的页面替换算法相比较而言是低效的，会出现Belady现象，但作为实验来说，它易于实现、调试。总体来说，虽然在实验中没有实现别的页面置换算法，但是同样复习了相关的理论知识，加深了对原理的理解。

### 列出你认为OS原理中很重要，但在实验中没有对应上的知识点
- 实验中没有讨论到Belady现象，当然由于在小作业lec中留了关于Belady的题，所以这里可能就没有当做重点考虑。
- 实验中没有涉及到磁盘对换区（用来保存本该在内存中的进程的存储空间）。



