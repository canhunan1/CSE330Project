/*  
 *  hello-1.c - The simplest kernel module.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/sched.h>    /* Needed for for_each_procee(), signal_struck and task_struck*/
#include <linux/tty.h>      /*Needed for tty_struct*/
#include <linux/jiffies.h>  /*utime, stime and HZ*/
#include <linux/cred.h>     /*Needed for cred (to get the uid for the user*/
#include <linux/string.h>



int __init modulep3(int argc, char **argv )
{
      int buffer_size = 0;
    int producer_num = 0;
    int comsumer_num = 0;
    int sleep_time = 0;
     if (argc <= 5){
         printk("Please input [buffer size], [producer number], [comsumer number], [sleep time]\n");

         return 0;
     }
     sscanf(argv[1], "%d", &buffer_size);
     sscanf(argv[2], "%d", &producer_num);
     sscanf(argv[3], "%d", &comsumer_num);
     sscanf(argv[4], "%d", &sleep_time);

    //create a buffer
    int buffer[buffer_size];
    printk(KERN_INFO "Hello world 1.\n");
	return 0;
}

void cleanup_module(void)
{
    printk(KERN_INFO "Goodbye world 1.\n");
}