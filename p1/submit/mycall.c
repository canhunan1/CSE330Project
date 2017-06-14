#include<linux/syscalls.h>
#include <linux/kernel.h>	/* Needed for KERN_INFO */

asmlinkage long sys_my_syscall(int i)

{
	printk(KERN_INFO "This is the new system call Jianan Yang implemented\n");


	return i+10;

}
