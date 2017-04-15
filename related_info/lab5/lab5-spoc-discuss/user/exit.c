#include <stdio.h>
#include <ulib.h>

int magic = -0x10384;

int
main(void) {
    int pid1,pid2,code;
    cprintf("I am the parent. Forking the child...\n\n");
    if ((pid1 = fork()) == 0) {
        cprintf("I am the child %d. step0\n",getpid());
        yield();
		cprintf("I am the child %d. step1\n",getpid());
        yield();
		cprintf("I am the child %d. step2\n",getpid());
        yield();
        exit(magic);
    }
    else {
		cprintf("I am parent, fork a child pid %d\n\n",pid1);
		if ((pid2 = fork()) == 0) {
			cprintf("I am the child %d. step0\n",getpid());
			yield();
			cprintf("I am the child %d. step1\n",getpid());
			yield();
			cprintf("I am the child %d. step2\n",getpid());
			yield();
			exit(magic);
		}
		else
			cprintf("I am parent, fork a child pid %d\n\n",pid2);
    }
    //assert(pid > 0);
    cprintf("I am the parent, waiting now..\n");
	wait();
    //assert(waitpid(pid, &code) == 0 && code == magic);
    //assert(waitpid(pid, &code) != 0 && wait() != 0);
    cprintf("wait pid %d and %d ok.\n", pid1,pid2);

    cprintf("exit pass.\n");
    return 0;
}

