/*
 Elevator Round Robin
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/blk_types.h>
#include <linux/kernel.h>

/* Data GLOBAL to the zfq_scheduler */
struct zfq_data
{
	/* The time slice length for every process request */
	unsigned int time_quantum;
	/* The current serving zfq_queue */
	struct zfq_queue *active_queue;
	/* The list head of a linked list of zfq_queue(s) */
	struct list_head zfqq_head;
	/* The time of a request dispatch*/
	struct timeval rq_start_time;	
	/* The time of a request completion */
	struct timeval rq_end_time;
	/* The flag that indicates whether the previously dispatched request is completed or not */
	int complete_flag;
};

/* Data LOCAL per-process queue */
struct zfq_queue
{
	/* The list head of a linked list of request from the process*/
	struct list_head requests;
	/* The list member of the zfq_queue linked list*/
	struct list_head lists;
	/* The PID of the process*/
	pid_t pid;
	/* The number of requests in this zfq_queue */
	int ref;
	/* The time quantum for the zfq_queue*/
	long time_quantum;
};

/*  global variables*/
struct zfq_data *lezfqd;
long int end_time, start_time;
struct request *rq; 

/* Create a queue for every process */
/* Used to allocate any elevator specific storage for a request */
/* rq the request being allocated */
static int zfq_set_req_fn(struct request_queue *q, struct request *rq, struct bio *my_bio, gfp_t gfp_mask)
{
	
	struct zfq_queue *zfqq;
	pid_t c_pid = current->pid;
    struct list_head *pos;
	int flag = 0;

	// we have to go through our zfq_queue list!
	list_for_each(pos, &lezfqd->zfqq_head)
	{
		zfqq = list_entry(pos, struct zfq_queue, lists);
		if( c_pid == zfqq->pid)  //if the process exists in queue
		{
			flag = 1;
			//rq->elv.priv[0] = zfqq;
			zfqq->ref ++;
			return 0;
		}
	}

  	if(!flag)  // If the process PID is not in existing zfq_queue list
    {    
		// create a new process queue
		zfqq = (struct zfq_queue *)kmalloc(sizeof(struct zfq_queue), gfp_mask);
		//struct zfq_queue *new = q->elevator->elevator_data;
		INIT_LIST_HEAD(&zfqq->requests);
		INIT_LIST_HEAD(&zfqq->lists);
		zfqq->pid = c_pid;
		zfqq->ref = 1;
		zfqq->time_quantum = (int)&lezfqd->time_quantum;		
				
		// add it to the end of the list 
		list_add_tail(&zfqq->lists, &lezfqd->zfqq_head);
		// mark the process associated with the queue. 
		  if(flag == 0) 
          {
                lezfqd->active_queue = zfqq;
                flag = 1;
           }
    }
		rq->elv.priv[0] = zfqq;

		printk("*******************\n\tIN set request\n***********************\n");
		return 0;	
}

/* Add a new I/O request into a queue */
/* Get the process queue for the request */
/* rq is the request to be added to the request_queue */
static void zfq_add_req_fn(struct request_queue *q, struct request *rq)
{
	//Get zfq_queue pointer from rq->elv.priv[0]
	struct zfq_queue *zfqq = rq->elv.priv[0];
	// queue the request into the request list
	list_add_tail(&rq->queuelist, &zfqq->requests);
	printk("********************\n\tIN add request\n***********************\n");
}

/* Find the process queue that we need to service and move a request from that to the dispatch list */
static int zfq_dispatch_fn(struct request_queue *q, int force) 
{
	//struct zfq_data *zfqd = q->elevator->elevator_data;

	if(lezfqd->complete_flag == 0) 
	{
		printk("*************************\n\tIN complete flag dispatch\n******************************\n");   
		return 0;	
	}
	else
	{
		// complete_flag is 1
		struct zfq_queue  *queue = lezfqd->active_queue;
				printk("*************************\n\tIN else complete flag dispatch\n******************************\n");
		// if the current active process’s time quantum is expired, move on to the next process 
		if(queue->time_quantum <= 0) // time quantum expired
		{
			// switch to next zfq_queue
			//list_move_tail(&zfqd->zfqq_head, & zfqd->zfqq_head);
			lezfqd->active_queue = list_entry(lezfqd->active_queue->lists.next, struct zfq_queue, lists);
			// start time slice for the new active_queue
			lezfqd->active_queue->time_quantum = lezfqd->time_quantum;
		}
		if(queue->ref != 0)
		{
			rq= list_entry(lezfqd->active_queue->requests.next, struct request, queuelist);
			// keep dispacthing from current zfq_queue (== active_queue)
			// remove one request from zfq_queue
			list_del(lezfqd->active_queue->requests.next);
			// dispatch request to a device request queue
			elv_dispatch_sort(q, rq);
			// record the dispacth time in rq_start_time
			do_gettimeofday(&(lezfqd->rq_start_time));
			lezfqd->complete_flag = 0;
			return 1;
		}
		else 
			{ return 0; }
	}
	printk("*************************\n\tIN dispatch\n******************************\n");
	return 0;
}

/* This will track the completion of a dispatched request */
/* track if the process queue is expired */
/* deduct the io time from the corresponding process' time quantum */
static void zfq_completed_req_fn(struct request_queue *q, struct request *rq)
{
	struct zfq_queue *zfqq =  rq->elv.priv[0];

	do_gettimeofday(&(lezfqd->rq_end_time));
	end_time = lezfqd->rq_end_time.tv_sec + (lezfqd->rq_end_time.tv_usec / 1000000);
	start_time = lezfqd->rq_start_time.tv_sec + (lezfqd->rq_start_time.tv_usec / 1000000);

	lezfqd->active_queue->time_quantum = lezfqd->active_queue->time_quantum - (end_time - start_time);
	lezfqd->complete_flag = 1;
	zfqq->ref--;
	printk("***********************\n\tIN completed_request\n*************************\n");
}

/* queue lock held here */
static void zfq_put_request(struct request *rq)
{
	struct zfq_queue *zfqq = rq->elv.priv[0];

	if((zfqq->ref) == 0 )
	{
		// active_queue is the one we need to free
		if(lezfqd->active_queue == zfqq)
		{
			// switch active_queue to the next zfq_queue
			 lezfqd->active_queue = list_entry(lezfqd->active_queue->lists.next, struct zfq_queue, lists);
		}
		list_del_init(&zfqq->lists);
		kfree(zfqq);
		rq->elv.priv[0] = NULL;
	}	
	printk("**************************\n\tIN put_request\n****************************\n");	
}

/* Elevator initialization function */
static int zfq_init_queue(struct request_queue *q, struct elevator_type *e)
{
    	lezfqd = kmalloc_node(sizeof(*lezfqd), GFP_KERNEL, q->node);
	
		if(!lezfqd) return NULL;
		lezfqd->time_quantum = 200;
		INIT_LIST_HEAD(&lezfqd->zfqq_head);
		lezfqd->complete_flag = 1;
		lezfqd->active_queue = NULL;
		printk("***********************\n\tThis is the lezfqd time_quantum %d\n*********************\n", lezfqd->time_quantum);
		return 0;
}

/* Elevator exit function */
static void zfq_exit_queue(struct elevator_queue *e)
{
    	struct zfq_data *zfqd = e->elevator_data;
		BUG_ON(!list_empty(&zfqd->zfqq_head));
    	kfree(zfqd);
}

/* I/O Scheduler */
static struct elevator_type elevator_zfq = {
    .ops = {
	.elevator_set_req_fn            	= zfq_set_req_fn,
	.elevator_add_req_fn            	= zfq_add_req_fn,
	.elevator_dispatch_fn        		= zfq_dispatch_fn,
	.elevator_completed_req_fn     		= zfq_completed_req_fn,
	.elevator_put_req_fn            	= zfq_put_request,

        /* Init, and exit functions to our module */
    .elevator_init_fn            		= zfq_init_queue,
    .elevator_exit_fn            		= zfq_exit_queue,
    },
    .elevator_name = "ZFQ Round Robin",
    .elevator_owner = THIS_MODULE,
};

/* Register the I/O scheduler into kernel */
static int __init zfq_init(void)
{
	return elv_register(&elevator_zfq);
}

/* Unregister the I/O scheduler from kernel */
static void __exit zfq_exit(void)
{
    	elv_unregister(&elevator_zfq);
}

module_init(zfq_init);
module_exit(zfq_exit);

MODULE_AUTHOR("Ana L. Hernandez, Wilfrido Rodriguez, & Artiom Tiurin");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Round-Robin IO scheduler");
