## lab1实验报告

#### 计45 李昊阳 2014011421

### 练习1：理解通过make生成执行文件的过程。

**1. 操作系统镜像文件ucore.img是如何一步一步生成的？(需要比较详细地解释Makefile中每一条相关命令和命令参数的含义，以及说明命令导致的结果)** 

答： 首先执行命令`man "V="`来了解通过make生成ucore.img时执行了哪些命令。可以发现这些命令大概分为两类：

第一类(以其中一条命令为例)：
	
	+ cc kern/init/init.c
	gcc -Ikern/init/ -fno-builtin -Wall -ggdb -m32 -gstabs -nostdinc  -fno-stack-protector -Ilibs/ -Ikern/debug/ -Ikern/driver/ -Ikern/trap/ -Ikern/mm/ -c kern/init/init.c -o obj/kern/init/init.o

这条命令及后续同类命令的作用是：**编译内核代码和BootLoader代码，生成.o文件以便后续的链接。**

其中命令参数的含义是：

- `-IDir`: 将Dir文件夹include进来，即告诉编译器寻找头文件的目录。这里DIR表示的目录有kern/init/; libs/; kern/debug/; kern/diver/; kern/trap/ ;kern/mm/；它的搜索顺序位于标准库之前
- `-fno-builtin`: 禁用gcc的built in函数进行优化，不承认不以 \_\_builtin_开头的函数为内建（built-in）函数。
- `-Wall`: 开启所有警告开关
- `-ggdb`: 使 GCC 为 GDB 生成专用的更为丰富的调试信息（但是，此时就不能用其他的调试器来进行调试了 ，如 ddx。）
- `-m32`: 生成32位代码
- `-gstabs`: 以stabs格式生成调试信息,不包括GDB调试信息
- `-nostdinc`: 不在标准系统目录中搜索头文件,只在-I指定的目录中搜索
- `-fno-stack-protector`: 不产生多余代码检查栈溢出
- `-c` : 指定被编译的源文件及路径
- `-o` : 指定生成目标文件及路径


第二类(仍以其中一条命令为例)：

	+ ld bin/kernel
	ld -m    elf_i386 -nostdlib -T tools/kernel.ld -o bin/kernel  obj/kern/init/init.o obj/kern/libs/stdio.o obj/kern/libs/readline.o obj/kern/debug/panic.o obj/kern/debug/kdebug.o obj/kern/debug/kmonitor.o obj/kern/driver/clock.o obj/kern/driver/console.o obj/kern/driver/picirq.o obj/kern/driver/intr.o obj/kern/trap/trap.o obj/kern/trap/vectors.o obj/kern/trap/trapentry.o obj/kern/mm/pmm.o  obj/libs/string.o obj/libs/printfmt.o

这条命令及后续同类命令的作用是：**链接上一步编译生成的文件。**

其中命令参数的含义是：

- `-m elf_i386`：设置为elf_i386模式（在64位的Linux下，gcc 编译 32 位程序需要添加参数 -m32  ，ld需要添加参数是 -m elf_i386）
- `-nostdlib`：不连接系统标准启动文件和标准库文件,只把指定的文件传递给连接器
- `-T tools/kernel.ld`：使用可置换标签文件tools/kernel.ld
- `-o xx.o`：指定一系列.o文件

第三类：
	
	dd

这条命令的作用是：**把内核和bootloader写入ucore.img内。**


总之，为了生成ucore.img，首先生成了kernel和bootblock。

**2.一个被系统认为是符合规范的硬盘主引导扇区的特征是什么？**

答：从`tool/sign.c`中可以知道一个被系统认为是符合规范的硬盘主引导扇区的特征是：

- 磁盘主引导扇区只有512字节。
- 第510，511个字节分别为标志性结束字节0x55和0xAA。


### 练习2：使用qemu执行并调试lab1中的软件。

- 从CPU加电后执行的第一条指令开始，单步跟踪BIOS的执行。
- 在初始化位置0x7c00设置实地址断点,测试断点正常。
- 从0x7c00开始跟踪代码运行,将单步跟踪反汇编得到的代码与bootasm.S和 bootblock.asm进行比较。
- 自己找一个bootloader或内核中的代码位置，设置断点并进行测试。

答：

#### 练习2.1 从CPU加电后执行的第一条指令开始，单步跟踪BIOS的执行。

修改`tools/gdbinit`文件为

	file bin/kernel
	target remote :1234
	set architecture i8086

执行`make debug`

执行`i registers`可以看到cs寄存器的值为0xf000，eip寄存器的值为0xfff0，即从第一条指令开始执行。后面通过`si`指令进行单步调试即可。

在gdb界面下，可通过如下命令来看BIOS的代码

	x /2i $pc  //显示当前eip处的汇编指令

在Makefile中增加下面的代码：
	
	lab1-mon: $(UCOREIMG)  
	$(V)$(TERMINAL) -e "$(QEMU) -S -s -d in_asm -D $(BINDIR)/q.log -monitor stdio -hda $< -serial null"  
	$(V)sleep 2  
	$(V)$(TERMINAL) -e "gdb -q -tui -x  tools/gdbinit"

这样在调用qemu时，依次执行的汇编指令便保存在了`q.log`中。

#### 练习2.2 在初始化位置0x7c00设置实地址断点,测试断点正常。

在`tool/gdbinit.c`中添加下面的指令，即在0x7c00设置断点并测试断点是否正常：
	
	b *0x7c00
	continue
	x /10i $pc

执行`make lab1-mon`，部分输出结果如下：

	The target architecture is assumed to be i8086
	Breakpoint 1 at 0x7c00
	
	Breakpoint 1, 0x00007c00 in ?? ()
	=> 0x7c00:      cli
	   0x7c01:      cld


可以看到断点设置正常。

#### 练习2.3 从0x7c00开始跟踪代码运行,将单步跟踪反汇编得到的代码与bootasm.S和 bootblock.asm进行比较。

执行`make lab1-mon`，在调用qemu时，依次执行的汇编指令保存在了`q.log`中。同样，也可以在0x7c00处输入指令`x /10i $pc`查看十条指令。

	=> 0x7c00:      cli
	   0x7c01:      cld
	   0x7c02:      xor    %ax,%ax
	   0x7c04:      mov    %ax,%ds
	   0x7c06:      mov    %ax,%es
	   0x7c08:      mov    %ax,%ss
	   0x7c0a:      in     $0x64,%al
	   0x7c0c:      test   $0x2,%al
	   0x7c0e:      jne    0x7c0a
	   0x7c10:      mov    $0xd1,%al

与boot/bootasm.S中的汇编代码对比，代码是一致的。

#### 练习2.4 自己找一个bootloader或内核中的代码位置，设置断点并进行测试

自己找了另一函数`kern_init`作为断点，测试方法和上述操作类似。

在tools/gdbinit中加入指令`b kern_init`，然后执行可以得到和之前类似的结果，断点设置正常。之后可以继续通过si命令单步调试。

	The target architecture is assumed to be i8086
	Breakpoint 1 at 0x100000: file kern/init/init.c, line 17.


### 练习3：分析bootloader进入保护模式的过程。

**BIOS将通过读取硬盘主引导扇区到内存，并转跳到对应内存中的位置执行bootloader。请分析bootloader是如何完成从实模式进入保护模式的。**

答：

主要分为以下几个过程

一开始处于实模式下，首先关中断并将一些寄存器重置为0。

	cli                                             # Disable interrupts
	cld                                             # String operations increment
	
	#Set up the important data segment registers (DS, ES, SS).
	xorw %ax, %ax                                   # Segment number zero
	movw %ax, %ds                                   # -> Data Segment
	movw %ax, %es                                   # -> Extra Segment
	movw %ax, %ss                                   # -> Stack Segment


打开A20，通过将键盘控制器上的A20线置于高电位，使得全部32条地址线可用，从而使80386突破1MB的访存限制，可以访问32位共4G的地址空间。

    # Enable A20:
    #  For backwards compatibility with the earliest PCs, physical
    #  address line 20 is tied low, so that addresses higher than
    #  1MB wrap around to zero by default. This code undoes this.
	seta20.1:
	    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
	    testb $0x2, %al
	    jnz seta20.1
	
	    movb $0xd1, %al                                 # 0xd1 -> port 0x64
	    outb %al, $0x64                                 # 0xd1 means: write data to 8042's P2 port

	seta20.2:
	    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
	    testb $0x2, %al
	    jnz seta20.2
	
	    movb $0xdf, %al                                 # 0xdf -> port 0x60
	    outb %al, $0x60                                 # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1


通过`lgdt gdtdesc`初始化GDT表。

通过`movl %cr0, %eax`将cr0寄存器的值保存到eax中。

通过`orl $CR0_PE_ON, %eax`设置保护模式的标志。

通过`movl %eax, %cr0`将保护模式标志存入cr0中。

通过`ljmp $PROT_MODE_CSEG, $protcseg`跳转到保护模式的第一条指令。

设置段寄存器，建立堆栈。最后成功转到保护模式，通过`call bootmain`进入boot主方法。

	.code32                                             # Assemble for 32-bit mode
	protcseg:
	    # Set up the protected-mode data segment registers
	    movw $PROT_MODE_DSEG, %ax                       # Our data segment selector
	    movw %ax, %ds                                   # -> DS: Data Segment
	    movw %ax, %es                                   # -> ES: Extra Segment
	    movw %ax, %fs                                   # -> FS
	    movw %ax, %gs                                   # -> GS
	    movw %ax, %ss                                   # -> SS: Stack Segment

	    # Set up the stack pointer and call into C. The stack region is from 0--start(0x7c00)
	    movl $0x0, %ebp
	    movl $start, %esp
	    call bootmain



**1.为何开启A20，以及如何开启A20？**

答：如果A20关闭，那么1MB以上地址空间将不可访问，为了访问1MB以上的地址空间所以需要开启A20。向键盘控制器8042发送一个命令。键盘控制器8042会将某个输出引脚的输出置高电平,作为A20地址线控制的输入。一旦设置成功之后,内存将不会再被绕回(memory wrapping),这样我们就可以寻址1MB以上的地址空间。

**2.如何初始化GDT表**

答：通过`lgdt gdtdesc`初始化GDT表。

**3.如何使能和进入保护模式**

答：使能的方法为：`%cr0 |= CR0_PE_ON`；通过`ljmp $PROT_MODE_CSEG, $protcseg`跳转到保护模式的第一条指令。


### 练习4：分析bootloader加载ELF格式的OS的过程。

**1.bootloader如何读取硬盘扇区的？**

答：在`bootmain`函数中调用`readseg`函数以读取第一页，然后在`readseg`函数中循环调用了真正读取硬盘扇区的`readsect`函数每次读一个扇区。在`readsect`函数中，读取扇区主要分为一些几步：

- 读I/O地址0x1F7,等待磁盘准备好。
- 调用一系列函数`outb()`，设置读取的扇区：发送读取扇区数值1到0x1F2端口；发送读取扇区编号的7-0位，15-8位，23-16位，27-24位分别到0x1F3,0x1F4,0x1F5端口以及0x1F6的低4位，并将0x1F6端口的高4位置为0xE0，表示指定主盘并使用LBA模式。向命令寄存器发送操作码0x20，表示要读取；
- 读I/O地址0x1F7,等待磁盘准备好。
- 调用函数`insl()`，从0x1F0端口读取硬盘数据。

`readseg`简单包装了readsect，可以从设备读取任意长度的内容。

在bootmain函数中，首先将硬盘上从第一个扇区开始的4096个字节读到内存中地址为0x10000处，然后检查ELF文件是否合法，并找到程序段的起始地址，读取内核程序到内存中，最后执行内核程序，实现代码如下：

	void bootmain(void) {
	    // read the 1st page off disk
	    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);
	
	    // is this a valid ELF?
	    if (ELFHDR->e_magic != ELF_MAGIC) {
	        goto bad;
	    }
	
	    struct proghdr *ph, *eph;
	
	    // load each program segment (ignores ph flags)
	    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
	    eph = ph + ELFHDR->e_phnum;
	    for (; ph < eph; ph ++) {
	        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
	    }
	
	    // call the entry point from the ELF header
	    // note: does not return
	    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();
	
	bad:
	    outw(0x8A00, 0x8A00);
	    outw(0x8A00, 0x8E00);
	
	    /* do nothing */
	    while (1);
	}


**2.bootloader是如何加载ELF格式的OS？**

答：bootloader是如何加载ELF格式的OS主要有以下几步：

- 调用函数`readseg()`从硬盘中读取ELF头文件；
- 判断ELF头文件是否合法，不合法直接跳转到bad部分，退出；
- ELF头部有描述ELF文件应加载到内存什么位置的描述表，读取出来将之存入`ph`；
- 按照程序头表的描述，将ELF文件中的数据载入内存；
- 根据ELF头表中的入口信息，找到内核的入口并开始运行；


### 练习5：实现函数调用堆栈跟踪函数

**我们需要在lab1中完成kdebug.c中函数print_stackframe的实现，可以通过函数print_stackframe来跟踪函数调用堆栈中记录的返回地址。**

答：修改`kdebug.c`中的`print_stackframe()`函数，首先读取ebp和eip的值，然后从0到`STACKFRAME_DEPTH`依次输出即可，实现过程可以参照文件中的注释部分。

	uint32_t ebp = read_ebp();
	uint32_t eip = read_eip();
	int i;
	for (i = 0; i < STACKFRAME_DEPTH; i++) {
		if (ebp == 0)
			break;
		cprintf("ebp:0x%08x eip:0x%08x ", ebp, eip);
		uint32_t* args = ((uint32_t*)ebp) + 2;
		cprintf("args:0x%08x 0x%08x 0x%08x 0x%08x\n", args[0], args[1], args[2], args[3]);
		print_debuginfo(eip-1);
        eip = ((uint32_t *)ebp)[1];
        ebp = ((uint32_t *)ebp)[0];
	}


执行`make qemu`后可以得到如下结果，和实验指导书中的结果类似。

	ebp:0x00007b38 eip:0x00100a28 args:0x00010094 0x00010094 0x00007b68 0x0010007f
	    kern/debug/kdebug.c:307: print_stackframe+22
	ebp:0x00007b48 eip:0x00100d1e args:0x00000000 0x00000000 0x00000000 0x00007bb8
	    kern/debug/kmonitor.c:125: mon_backtrace+10
	ebp:0x00007b68 eip:0x0010007f args:0x00000000 0x00007b90 0xffff0000 0x00007b94
	    kern/init/init.c:48: grade_backtrace2+19
	ebp:0x00007b88 eip:0x001000a1 args:0x00000000 0xffff0000 0x00007bb4 0x00000029
	    kern/init/init.c:53: grade_backtrace1+27
	ebp:0x00007ba8 eip:0x001000be args:0x00000000 0x00100000 0xffff0000 0x00100043
	    kern/init/init.c:58: grade_backtrace0+19
	ebp:0x00007bc8 eip:0x001000df args:0x00000000 0x00000000 0x00000000 0x00103260
	    kern/init/init.c:63: grade_backtrace+26
	ebp:0x00007be8 eip:0x00100050 args:0x00000000 0x00000000 0x00000000 0x00007c4f
	    kern/init/init.c:28: kern_init+79
	ebp:0x00007bf8 eip:0x00007d6e args:0xc031fcfa 0xc08ed88e 0x64e4d08e 0xfa7502a8
	    <unknow>: -- 0x00007d6d --

输出信息依次为栈中每层的ebp和eip的值，ebp指向当前函数调用栈中栈底的位置，eip为下一条指令的地址。接下来输出函数的（四个）参数。

最后一行输出的ebp为0x00007bf8，0x00007d6e，这时因为bootloader被加载到了0x00007c00地址处，在执行到bootasm最后"call bootmain"指令时，首先将返回地址压栈，再将当前ebp压栈，所以此时esp为0x00007bf8。在bootmain函数入口处，有mov %esp %ebp指令，故bootmain中ebp为0x00007bf8。




### 练习6：完善中断初始化和处理 

**1.中断描述符表（也可简称为保护模式下的中断向量表）中一个表项占多少字节？其中哪几位代表中断处理代码的入口？**

答：中断描述符表中一个表项占8字节。在一个8字节（64位）的表项中，63-48位表示段内偏移的高16位，31-16位表示16位段选择子，15-0位表示段内偏移的低16位。入口地址=段选择子+段内偏移

**2.请编程完善kern/trap/trap.c中对中断向量表进行初始化的函数idt_init。在idt_init函数中，依次对所有中断入口进行初始化。使用mmu.h中的SETGATE宏，填充idt数组内容。每个中断的入口由tools/vectors.c生成，使用trap.c中声明的vectors数组即可。**

答：具体的代码实现，参见`kern/trap/trap.c`实现过程主要有以下几步：

- 声明__vertors[],其中存放着中断服务程序的入口地址（这个数组生成于vertor.S中）。
- 填充中断描述符表IDT
- 使用lidt指令加载中断描述符表


**3.请编程完善trap.c中的中断处理函数trap，在对时钟中断进行处理的部分填写trap函数中处理时钟中断的部分，使操作系统每遇到100次时钟中断后，调用print_ticks子程序，向屏幕上打印一行文字”100 ticks”。**

答：为了让操作系统每遇到100次时钟中断后调用print_ticks子程序，在`case IRQ_OFFSET + IRQ_TIMER`中增加下列代码即可：

		ticks++;
		if (ticks % TICK_NUM == 0) {
			print_ticks();
		}
        break;

### 扩展练习 Challenge 1

**扩展proj4,增加syscall功能，即增加一用户态函数（可执行一特定系统调用：获得时钟计数值），当内核初始完毕后，可从内核态返回到用户态的函数，而用户态的函数又通过系统调用得到内核态的服务**

答：首先需要在`idt_init`将用户态调用SWITCH_TOK中断的权限打开：
	
	SETGATE(idt[T_SWITCH_TOK], 0, GD_KTEXT, __vectors[T_SWITCH_TOK], DPL_USER);

然后在`trap_dispatch`中，修改iret时会从堆栈弹出的段寄存器。在lab1\_switch\_to\_user中，调用T\_SWITCH\_TOU中断。因为从中断返回时，会多pop两位，并用这两位的值更新ss,sp，损坏堆栈。所以要先把栈压两位，并在从中断返回后修复esp。需要在`kern/init/init.c`中增加：

	asm volatile (
	    "sub $0x8, %%esp \n"
	    "int %0 \n"
	    "movl %%ebp, %%esp"
	    : 
	    : "i"(T_SWITCH_TOU)
	);


在lab1\_switch\_to_\kernel中，调用T\_SWITCH\_TOK中断。从中断返回时，esp仍在TSS指示的堆栈中。所以要在从中断返回后修复esp。需要在`kern/init/init.c`中增加：

	asm volatile (
	    "int %0 \n"
	    "movl %%ebp, %%esp \n"
	    : 
	    : "i"(T_SWITCH_TOK)
	);

具体实现参见代码。


### 列出本实验各练习中对应的OS原理的知识点

- makefile的基本写法
- gdb的使用
- 开机启动的过程，从BIOS加载bootloader再加载ucore内核的过程
- ucore中对中断的支持
		


