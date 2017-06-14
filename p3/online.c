#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/delay.h>

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>



MODULE_LICENSE("GPL");
MODULE_AUTHOR("SungHo Hong");


// semaphore & mutex
struct semaphore sem, empty, full;
struct mutex mut;
static int  counter, *buffer, buffer_counter=0, kill_signal_lock=0;
struct task_struct **threads;
static int ki_count = 0;



//  argument data 
static int buffers, producers, consumers, sleep;
module_param(buffers, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(producers, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(consumers, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(sleep, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);



static void initialize_data(int buffers, int consumers, int producers ){

	buffer = (int *)kmalloc(buffers * sizeof(int), GFP_KERNEL);
        threads = kmalloc((consumers+producers)*sizeof(*threads), GFP_KERNEL);
        
        sema_init(&empty, buffers);
        sema_init(&full, buffers);
     	mutex_init(&mut);

        counter = 0;	
}


static int consumer(void *param){
	char *output = (char *)param;
	printk(KERN_INFO "%s created \n",output);
   
        allow_signal(SIGKILL);     
	while(!kthread_should_stop()){
		
		//ssleep(sleep);
                
		if(kill_signal_lock==0){ 
               
		ssleep(sleep); 
                if(down_interruptible(&full)){
			break;
		}

                mutex_lock(&mut);
		// ssleep(sleep);

		// CRITICAL SECTION BEGIN
                
                if(buffer_counter > 0){ 
                     printk(KERN_INFO "Consumed[%d]\n",output, buffer[ buffer_counter-1 ]);
                     buffer_counter--;
                }

                // CRITICAL SECTION END


                mutex_unlock(&mut);
                up(&empty); 
	
		 }  else {
                         ssleep(1);
                }
	
	}

	ki_count++;
	do_exit(0);
	return 0;
}


static int producer(void *param){
        char *output = (char *)param;
	printk(KERN_INFO "%s created \n",output);

        allow_signal(SIGKILL);
	while(!kthread_should_stop()){
              // ssleep(sleep);
            
               if(kill_signal_lock==0){ 
                  		  
                 ssleep(sleep);
		 if(down_interruptible(&empty)){
			break;
	          } 
                	 	               
                 mutex_lock(&mut);

                // CRITICAL SECTION BEGIN

		if(buffer_counter < buffers){
               		 printk(KERN_INFO "%s  produced[%d] \n",output, counter);
                 	 buffer[ buffer_counter ] = counter;
                         counter++; buffer_counter++;
		}
