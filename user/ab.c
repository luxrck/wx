// AB
#include <stdio.h>
#include <lib/libc.h>

int main(int argc, char **argv)
{
	if (sys_fork() == 0) {
		for (int i=0; i<52; i++)
			sys_a();
	} else {
		for (int i=0; i<50; i++)
			sys_b();
	}
}
