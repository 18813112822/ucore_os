#include <defs.h>
#include <syscall.h>
#include <stdio.h>
#include <ulib.h>

void
exit(int error_code) {
    sys_exit(error_code);
    cprintf("BUG: exit failed.\n");
    while (1);
}

int
fork(void) {
	cprintf("\n++++syscall fork ,from user to kernel\n");
	int a=sys_fork();
	cprintf("++++syscall fork end,from kernel to user\n\n");
    return a;
	
}

int
wait(void) {
	cprintf("\n++++syscall wait ,from user to kernel\n");
	int a=sys_wait(0, NULL);
	cprintf("++++syscall wait end,from kernel to user\n\n");
    return a;
}

int
waitpid(int pid, int *store) {
    return sys_wait(pid, store);
}

void
yield(void) {
	cprintf("\n++++++syscall yield,switch user to kernel \n");
    sys_yield();
	cprintf("++++++syscall yield end ,switch kernel to user \n\n");
}

int
kill(int pid) {
    return sys_kill(pid);
}

int
getpid(void) {
    return sys_getpid();
}

//print_pgdir - print the PDT&PT
void
print_pgdir(void) {
    sys_pgdir();
}

