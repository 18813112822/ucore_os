## lab2实验报告

#### 计45 李昊阳 2014011421

### 练习0：填写已有实验

**本实验依赖实验1。请把你做的实验1的代码填入本实验中代码中有“LAB1”的注释相应部分。** 

答： 这里我使用了图形化的比较/merge工具meld来手动合并。

### 练习1：实现 first-fit 连续物理内存分配算法（需要编程）

**在实现first fit 内存分配算法的回收函数时，要考虑地址连续的空闲块之间的合并操作。提示:在建立空闲页块链表时，需要按照空闲页块起始地址来排序，形成一个有序的链表。** 

答：我修改了`default_pmm.c`中的`default_alloc_pages`函数和`default_free_pages`函数，从而实现了`first fit`内存分配算法。


**分配过程：** 

需要修改`default_alloc_pages`函数，修改后如下：


```c
static struct Page *
default_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) { 
        return NULL;
    }
    struct Page *page = NULL;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        if (p->property >= n) {
            page = p;
            break;
        }
    }
    if (page != NULL) {
        struct Page *p = page;
        for(; p != page + n; p++) {
            ClearPageProperty(p);
            SetPageReserved(p);
        }
        if (page->property > n) {
            p = page + n; //get pointer of left space
            p->property = page->property - n;
            SetPageProperty(p); // set size of left space
            page->property = n;
            list_add(&page->page_link, &(p->page_link));
        }
        list_del(&(page->page_link));
        nr_free -= n;

    	/**
    	list_del(&(page->page_link));
        if (page->property > n) {
            struct Page *p = page + n;
            p->property = page->property - n;
            list_add(&free_list, &(p->page_link));
        }
        nr_free -= n;
        ClearPageProperty(page);
        */
    }
    return page;
}

```

简要说明设计实现过程：

- 检查需要分配的内存空间是否超过空闲空间，如果超过直接返回NULL，分配失败。
- 从用来保存空闲内存空间的列表`free_list`开始遍历链表，找到第一块空间大于等于所需空间的内存块。
- 分配连续n页，设置相应页的标志位，表示将其分配出去。
- 将分配出去的页从链表中删除。
- 如果分配的空间大于所需空间那么将剩余的页重新加入链表。

**释放过程：**

需要修改`default_free_pages`函数，修改后如下：

```c
static void
default_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p) && !PageProperty(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    list_entry_t *le = list_next(&free_list);

    while (le != &free_list) {
    	p = le2page(le, page_link);
		if(p > base)
			break;
		le = list_next(le);
	}

	list_add_before(le, &(base->page_link));

	p = le2page(le, page_link);
	if(le != &free_list && base + base->property == p) {
		base->property += p->property;
		ClearPageProperty(p);
		list_del(&(p->page_link));
	}

	le = list_prev(&(base->page_link));
	p = le2page(le, page_link);
	if (le != &free_list && p + p->property == base) {
		p->property += base->property;
		ClearPageProperty(base);
		list_del(&(base->page_link));
	}
	nr_free += n;

    /**
    while (le != &free_list) {
        p = le2page(le, page_link);
        le = list_next(le);
        if (base + base->property == p) {
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
        }
        else if (p + p->property == base) {
            p->property += base->property;
            ClearPageProperty(base);
            base = p;
            list_del(&(p->page_link));
        }
    }
    nr_free += n;
    list_add(&free_list, &(base->page_link));
    */
}
```

简要说明设计实现过程：
- 修改待释放的页的标志位；
- 在链表中找到释放后的空间应该加入的位置并将其加入链表；
- 判断该区域是否可以和低地址空闲空间合并，如果可以则合并成更大空间；
- 判断该区域是否可以和高地址空闲空间合并，如果可以则合并成更大空间。


**问：你的first fit算法是否有进一步的改进空间**

答：如果采用平衡树来维护空闲地址空间，那么在分配和回收时，查找操作可以降低复杂度，从而更快。



### 练习2：实现寻找虚拟地址对应的页表项（需要编程）

答：需要修改`kern/mm/pmm.c`中的`get_pte`函数，修改后如下：

```c
    pde_t *pdep = &pgdir[PDX(la)]; // (1) find page directory entry
    if (!(*pdep & PTE_P)) { // (2) check if entry is not present
        struct Page *page;
        if (!create || (page = alloc_page()) == NULL) {// (3) check if creating is needed, then alloc page for page table
            return NULL;
        }
        set_page_ref(page, 1); // (4) set page reference
        uintptr_t pa = page2pa(page); // (5) get linear address of page
        memset(KADDR(pa), 0, PGSIZE); // (6) clear page content using memset
        *pdep = pa | PTE_U | PTE_W | PTE_P; // (7) set page directory entry's permission
    }
    return &((pte_t *)KADDR(PDE_ADDR(*pdep)))[PTX(la)]; // (8) return page table entry
```

按照注释一步一步实现即可，简要说明设计实现过程：
- 根据虚地址的高十位查询页目录，找到页表项的`pdep`
- 判断`(pgdir[PDX(la)] & PTE_P)`是否为真
	- 如果为真，则根据虚拟地址的中间十位，找到虚拟地址对应的页表项返回`(pte_t*)KADDR(PTE_ADDR(*pdep)) + PTX(la)`
	- 否则，若create参数为1，则为PTE申请一个页，调用memset，设置该页的ref值为1，填入PDE的值，最后返回结果；而如果create为0，则直接返回NULL。

#### 问题2.1

**请描述页目录项（Pag Director Entry）和页表（Page Table Entry）中每个组成部分的含义和以及对ucore而言的潜在用处。**

答：

PDE和PTE的组成如下：

PDE的组成部分：
- 高20位：页表的物理地址（物理页帧号）
- 低12位：标志位
	- 第5位：访问位，表示是否被访问过；选择换出的页时会用到
	- 第4位：禁用缓存位，对于IO设备会用到
	- 第3位：写权限控制
	- 第2位：用户态程序访问权限控制，但ucore设计当中似乎倾向于将这一控制推迟到二级页表，而在PDE当中不进行
	- 第1位：读写权限控制
	- 第0位：存在位，ucore当中用来判断对应的二级页表是否被分配了物理内存

PTE和PDE基本相同，但多了一位会被用到的，第6位（Dirty标志位，表征是否被写入过数据，这决定在将该页换出时是否把数据写回外部存储）。

标志位众多，有：
```c
#define PTE_P           0x001                   // Present
#define PTE_W           0x002                   // Writeable
#define PTE_U           0x004                   // User
#define PTE_PWT         0x008                   // Write-Through
#define PTE_PCD         0x010                   // Cache-Disable
#define PTE_A           0x020                   // Accessed
#define PTE_D           0x040                   // Dirty
#define PTE_PS          0x080                   // Page Size
#define PTE_MBZ         0x180                   // Bits must be zero
#define PTE_AVAIL       0xE00                   // Available for software use
                                                // The PTE_AVAIL bits aren't used by the kernel or interpreted by the
                                                // hardware, so user processes are allowed to set them arbitrarily.
```

对ucore的潜在用途：用于信息记录，如标志位的access位可用于clock算法的实现

**如果ucore执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？**
答：如果出现页访问异常，硬件会触发中断，并跳转到指定的中断服务例程，使操作系统处理中断。大概的过程是：

1. 判断是否有空闲的页，如果有，跳到4；
2. 没有空闲的页，则需要换出，选择一个页，如果被写入过，将新的内容写入外部存储；
3. 换出该页，并修改对应页表项的存在位；
4. 将需要装入的页写入内存，并需要对应页表项的存在位。
5. 重新执行触发异常的指令。


### 练习3：释放某虚地址所在的页并取消对应二级页表项的映射（需要编程）
答：需要修改`kern/mm/pmm.c`中的`page_remove_pte`函数，修改后如下：

```c
    if (*ptep & PTE_P) { //(1) check if this page table entry is present
        struct Page *page = pte2page(*ptep); //(2) find corresponding page to pte
        if (page_ref_dec(page) == 0) { //(3) decrease page reference
            free_page(page);//(4) and free this page when page reference reachs 0
        }
        *ptep = 0; //(5) clear second page table entry
        tlb_invalidate(pgdir, la); //(6) flush tlb
    }
```

同样按照注释一步一步实现即可，简要说明设计实现过程：
- 首先判断对应的二级页表是否存在
- 找到页表项对应的物理页帧
- 将页的对应的被引用次数减一
- 如果ref为0，释放该页
- 清除页表项，
- 调用tlb_invalidate，使对页表的修改生效，更新tlb

#### 问题3.1
**数据结构Page的全局变量（其实是一个数组）的每一项与页表中的页目录项和页表项有无对应关系？如果有，其对应关系是啥？**
答：
在`pmm.c`的`page_init`函数中,有下面这部分代码：
```c
    npage = maxpa / PGSIZE;
    pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);

    for (i = 0; i < npage; i ++) {
        SetPageReserved(pages + i);
    }

    uintptr_t freemem = PADDR((uintptr_t)pages + sizeof(struct Page) * npage);
```
可以看到，首先计算出页的总数npage，然后分配内存pages为Page数组的头指针，并对npage项初始化。可见每一项对应物理空间的每一页，当一个二级页表映射到某个物理页，其所存储的物理地址（高20位）与某个page的page2pa(page)值相同。


#### 问题3.2
**如果希望虚拟地址与物理地址相等，则需要如何修改lab2，完成此事？**
答：
可以通过以下步骤实现：
- 修改memlayout.h：
```c
#define KERNBASE     0x00000000
#define VPT          0x3AC00000
```
- 修改kernel.ld：
```c
. = 0x00100000;
```
- 在pmm.c中去掉置boot_pgdir[0]为0的项，因为此时虚地址与实地址一一对应：
```c
//assert(boot_pgdir[0] == 0);
//boot_pgdir[0] = 0;
```

		
### 分析ucore_lab中提供的参考答案，并请在实验报告中说明你的实现与参考答案的区别
答：我的实现与参考答案的区别着重体现在练习一中，即first-fit 连续物理内存分配算法的实现不同。
- 我的实现：将一块连续的物理空间的第一页插入链表，并记录这块地址空间的大小，分配、释放空间的执行效率较高。
- 答案的实现：将物理空间的所有空闲页面都插入了双向链表，分配、释放时所需的开销更大。

这就是我的实现和参考答案的主要区别。练习2和3的实现和参考答案的思路相同，直接按照注释的提示一步步完成即可。


### 列出你认为本实验中重要的知识点，以及与对应的OS原理中的知识点，并简要说明你对二者的含义，关系，差异等方面的理解（也可能出现实验中的知识点没有对应的原理知识点）

答：我认为重要的知识点及对应的os原理中的知识点如下：
- 练习一：
	- 连续物理内存分配的first-fit算法，对应os原理课的连续内存分配；
	- 页的标志位的含义；
	- 以页为单位管理物理内存：原理课中介绍了以页为单位管理内存的原理，但是具体实现时还有更多细节，比如系统需要建立相应的数据结构Page来管理物理页。
	 
- 练习二：
	- 线性地址与实地址的转换过程以及页表的建立和索引。
	- 页目录表、页表结构：os原理课上讲对于一个给定的虚拟地址，高十位加上页目录表基址找到对应的页表基址，找到的页表基址加上虚拟地址的中间十位找到对应的页表项。实验时页表和页目录表是个动态建立的过程，页目录表某一项对应的页表可能未分配页，这点需要在实验中注意到。
	
- 练习三：页表的释放，与os原理课页表的释放算法相对应

我认为实验加深了我们对理论知识的理解，培养了我们将理论转化为实际代码的能力。

### 列出你认为OS原理中很重要，但在实验中没有对应上的知识点

答：我认为OS原理课中讲到的best-fit和worst-fit算法没有在实验中实践到，还有比如反置页表也没有在实验中体现，这些都是很重要的知识点。还有比如操作系统是如何知道物理内存大小的？页目录表基址如何获得的？（建立页机制时通过lcr3指令把页目录表的起始地址存入CR3寄存器中，需要时读出）若该页表的所有pte项都被释放，则该二级页表是否需要释放？对应的PDE项是否需要取消映射？等等这些问题都很重要，但在实验中如果不想清楚地话就不能很好地学习到这部分知识点。