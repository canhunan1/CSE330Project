/*  
 *  hello-1.c - The simplest kernel module.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JIANAN YANG");

static int buffer_size, producer_num, consumer_num, sleep_time;
module_param(buffer_size, int, S_IRUSR | S_IWUSR );
module_param(producer_num, int, S_IRUSR | S_IWUSR );
module_param(consumer_num, int, S_IRUSR | S_IWUSR );
module_param(sleep_time, int, S_IRUSR | S_IWUSR );
struct semaphore n;
struct semaphore mutex;
struct semaphore empty;
struct semaphore full;
//struct task_struct *producer_thread;
//struct task_struct *consumer_thread;

struct task_struct **producer_threads;
struct task_struct **consumer_threads;
int* buffer;

int STOP = 0;

int counter = 0;
static int producer(void *data);
static int consumer(void *data);

static int __init module_project3_init(void)
{

    printk(KERN_INFO "buffer size: %d, producer_num: %d, consumer_num: %d, sleep_time: %d\n", buffer_size, producer_num, consumer_num, sleep_time);

    //create a buffer
    //int data_pro;
    //int data_con;
    sema_init(&n, buffer_size);
    sema_init(&mutex, 1);
    sema_init(&empty, buffer_size);
    sema_init(&full, 0);

    buffer = (int *)kmalloc(buffer_size * sizeof(int), GFP_KERNEL);
    memset(buffer,0,buffer_size * sizeof(int));

    //producer_thread = kthread_run(&producer, (void*)p1, "thread_func_1");
    //consumer_thread = kthread_run(&consumer, (void*)p2, "thread_func_2");
    producer_threads = kmalloc(producer_num*sizeof(struct task_struct*), GFP_KERNEL);
    consumer_threads = kmalloc(consumer_num*sizeof(struct task_struct*), GFP_KERNEL);
    int i = 0;
    for(; i < producer_num; i++){
        //producer_threads[i] = (struct task_struct *) kmalloc(sizeof(struct task_struct*), GFP_KERNEL);
        //memset(producer_threads[i],0,sizeof(struct task_struct));
        int data_pro = i;
        producer_threads[i] = (struct task_struct *) kthread_run(&producer, (void*)(&data_pro), "producer");
        if(!IS_ERR(producer_threads[i])){
            printk(KERN_INFO "Producer thread[%d] successfully", i);
        }else{
            int err = PTR_ERR(producer_threads[i]);
            printk(KERN_INFO "Producer thread[%d] Run Error %d", err);
        }
    }
    int j = 0;
    for(; j < consumer_num; j++){
        //consumer_threads[j] =  kmalloc(sizeof(struct task_struct), GFP_KERNEL);
        //memset(consumer_threads[j],0,sizeof(struct task_struct));
        int data_con = j;
        consumer_threads[j] = (struct task_struct *) kthread_run(&consumer, (void*)(&data_con), "consumer");
        if(!IS_ERR(consumer_threads[j])){
            printk(KERN_INFO "Consumer thread[%d] successfully", j);
        }else{
            int err = PTR_ERR(consumer_threads[j]);
            printk(KERN_INFO "Consumer thread[%d] Run Error %d", err);
        }
    }

	return 0;
}

static int producer(void *data){
    int * data_int = (int *)data;
    int thread_num = *data_int;

    while(!kthread_should_stop()) {

        if(down_interruptible(&empty)) {
            printk(KERN_INFO "Error::producer::could not hold semaphore empty");
            return -1;
        }
        if(down_interruptible(&mutex)) {
            printk(KERN_INFO "Error::producer:: could not hold semaphore mutex");
            return -1;
        }




        if(STOP == 0) {
            printk(KERN_INFO "Produced[%d] \n ", counter);
            buffer[counter] = counter;
            counter++;
        }else {
            up(&empty);// all the producer thread has been stopped.
        }

        up(&mutex);
        up(&full);
        msleep(sleep_time);

    }

    return 0;
}

static int consumer(void *data) {
    int * data_int = (int *)data;
    int thread_num = *data_int;

    while (!kthread_should_stop()){

        if(down_interruptible(&full)) {
            printk(KERN_INFO "Error::consumer::could not hold semaphore full");
            return -1;
        }
        if(down_interruptible(&mutex)) {
            printk(KERN_INFO "Error::consumer::could not hold semaphore mutex");
            return -1;
        }

        if(STOP == 0) {
            //printk(KERN_INFO "Thread[%d]\tConsumed[%d]\n", thread_num, buffer[--counter]);
            printk(KERN_INFO "Consumed[%d] \n ", buffer[--counter]);
        }else {
            up(&full);// all the producer thread has been stopped.
        }

        up(&mutex);
        up(&empty);
        msleep(sleep_time);
    }

    return 0;
}




static void __exit module_project3_exit(void)
{
    printk(KERN_INFO "Goodbye world 1.\n");
    STOP = 1;
    up(&empty);
    int i = 0;
    printk(KERN_INFO "producer_num %d", producer_num);
    for(; i < producer_num; i++){
        printk(KERN_INFO "producer loop %d", i);
        up(&empty);
        int res = kthread_stop((struct task_struct *)producer_threads[i]);
        if(res != -EINTR) {
            printk(KERN_INFO "Producer thread[%d] has been stopped", i);
        }else{
            int err = PTR_ERR(producer_threads[i]);
            printk(KERN_INFO "Producer thread[%d] STOP Error %d", err);
        }
        //kfree((struct task_struct *)producer_threads[i]);
    }
    int j = 0;
    printk(KERN_INFO "consumer_num %d", producer_num);

    up(&full);
    for(; j < consumer_num; j++){
        printk(KERN_INFO "consumer loop %d", j);

        int res = kthread_stop((struct task_struct *)consumer_threads[j]);
        if(res != -EINTR){
            printk(KERN_INFO "Consumer thread[%d] has been stopped", j);
        }else{
            int err = PTR_ERR(consumer_threads[j]);
            printk(KERN_INFO "Consumer thread[%d] STOP Error %d", err);
        }
       // kfree((struct task_struct *)consumer_threads[j]);
    }
    kfree(buffer);
    kfree(producer_threads);
    kfree(consumer_threads);
}

module_init(module_project3_init);
module_exit(module_project3_exit);

