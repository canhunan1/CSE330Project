#include <linux/unistd.h>
#include<stdio.h>
#define __NR_my_syscall  548
int main()
{
	printf("%d\n",syscall(__NR_my_syscall,10));
	return 0;
}
