// hello, world
#include <stdio.h>
#include <lib/libc.h>

time_t tm = 0;

int main(int argc, char **argv)
{
	if (sys_fork() == 0) {
		if (sys_fork() == 0) {
			//sys_a();
			for (int i=0; i<100000; i++);
			sys_exit();
		}
		else {
			//sys_b();
			sys_waitpid(-1);
			sys_exit();
		}
	} else {
		sys_hello();
		sys_waitpid(-1);
		sys_exit();
	}
}
