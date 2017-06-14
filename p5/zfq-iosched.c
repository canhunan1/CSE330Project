/*
 *  hello-1.c - The simplest kernel module.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/slab.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JIANAN YANG");


struct zfq_data {
	unsigned int time_quantum;			//The time slice length for every process queue
	struct zfq_queue *active_queue;		//The current serving zfq_queue
	struct list_head zfqq_head;			//The list head of a linked list of zfq_queue
	struct timeval rq_start_time;		//The time of a request dispatch
	struct timeval rq_end_time;			//The time of a request completion
	int complete_flag;					//The flag that indicates whether the previous dispatch request is completed or not
};

struct zfq_queue{
	struct list_head requests;			//The list head of a linked list of request from the process
	struct list_head lists;				//The list member of the zfq_quque linked list
	pid_t pid;							//PID of the process
	int ref;							//The number of requests in this zfq_queue
	long long time_quantum;				//The time quantum for the zfq_queue
};


static int zfq_set_req_fn(struct request_queue *q, struct request *rq, struct bio *my_bio, gfp_t gfp_mask){
	struct zfq_data *zfqd = q->elevator->elevator_data;
	struct zfq_queue *zfqq;
	pid_t c_pid = current->pid;
    struct list_head *pos;
	int is_exist = 0;
	list_for_each(pos, &zfqd->zfqq_head){
		zfqq = list_entry(pos, struct zfq_queue, lists);
		if( c_pid == zfqq->pid){
			is_exist = 1;
			break;
		}
	}
	if(is_exist == 0){
		zfqq = (struct zfq_queue *)kmalloc(sizeof(struct zfq_queue), gfp_mask);
		INIT_LIST_HEAD(&zfqq->requests);
		INIT_LIST_HEAD(&zfqq->lists);
		zfqq->pid = c_pid;
		zfqq->time_quantum = (int)&zfqd->time_quantum;
		list_add_tail(&zfqq->lists, &zfqd->zfqq_head);						
	}
	zfqq->ref++;
	rq->elv.priv[0] = zfqq;
	return 0;	
}


static void zfq_add_req_fn(struct request_queue *q, struct request *rq){
	struct zfq_queue *zfqq = rq->elv.priv[0];
	list_add_tail(&rq->queuelist, &zfqq->requests);
}

static int zfq_dispatch_fn(struct request_queue *q, int force) {
	struct zfq_data *zfqd = q->elevator->elevator_data;
	if(zfqd->complete_flag == 0) 
		return 0;	
	else{
		struct zfq_queue  *queue = zfqd->active_queue;
		if(queue->time_quantum <= 0){//no time left for this queue, need to switch to another queue
			zfqd->active_queue = list_entry(zfqd->active_queue->lists.next, struct zfq_queue, lists);
			zfqd->active_queue->time_quantum = zfqd->time_quantum;
		}
		if(queue->ref != 0){
			struct request *rq= list_entry(zfqd->active_queue->requests.next, struct request, queuelist);
			list_del(zfqd->active_queue->requests.next);
			elv_dispatch_sort(q, rq);
			do_gettimeofday(&(zfqd->rq_start_time));
			zfqd->complete_flag = 0;
			return 1;
		}
	}
	return 0;
}

static void zfq_completed_req_fn(struct request_queue *q, struct request *rq){
	struct zfq_queue *zfqq =  rq->elv.priv[0];
	struct zfq_data *zfqd = q->elevator->elevator_data;
	do_gettimeofday(&(zfqd->rq_end_time));
	long long end_time = zfqd->rq_end_time.tv_sec + (zfqd->rq_end_time.tv_usec / 1000000);
	long long start_time = zfqd->rq_start_time.tv_sec + (zfqd->rq_start_time.tv_usec / 1000000);
	zfqd->active_queue->time_quantum = zfqd->active_queue->time_quantum - (end_time - start_time);
	zfqd->complete_flag = 1;
	zfqq->ref--;
}

static void zfq_put_req_fn(struct request *rq){
	struct zfq_queue *zfqq = rq->elv.priv[0];
	if(zfqq->ref == 0 ){
		list_del_init(&zfqq->lists);
		kfree(zfqq);
		rq->elv.priv[0] = NULL;
	}	
}

/* Elevator initialization function */
static int zfq_init_queue(struct request_queue *q, struct elevator_type *e){
		struct zfq_data *zfqd;
		struct elevator_queue *eq;
		eq = elevator_alloc(q, e);
		if (!eq)
			return -ENOMEM;
    	zfqd = kmalloc_node(sizeof(*zfqd), GFP_KERNEL, q->node);
	
		if(!zfqd) {
			kobject_put(&eq->kobj);
			return -ENOMEM;
		}
		zfqd->time_quantum = 100;
		zfqd->complete_flag = 1;
		zfqd->active_queue = NULL;
		eq->elevator_data = zfqd;
		INIT_LIST_HEAD(&zfqd->zfqq_head);
		spin_lock_irq(q->queue_lock);
		q->elevator = eq;
		spin_unlock_irq(q->queue_lock);
		return 0;
}

/* Elevator exit function */
static void zfq_exit_queue(struct elevator_queue *e){
    	struct zfq_data *zfqd = e->elevator_data;
		BUG_ON(!list_empty(&zfqd->zfqq_head));
    	kfree(zfqd);
}

static struct elevator_type elevator_zfq = {
	.ops = {
		.elevator_dispatch_fn		= zfq_dispatch_fn,
		.elevator_add_req_fn		= zfq_add_req_fn,
        .elevator_set_req_fn        = zfq_set_req_fn,
        .elevator_put_req_fn        = zfq_put_req_fn,
        .elevator_completed_req_fn  = zfq_completed_req_fn,
        .elevator_init_fn		    = zfq_init_queue,
		.elevator_exit_fn		    = zfq_exit_queue,
	},
	.elevator_name = "zfq",
	.elevator_owner = THIS_MODULE,
};


static int __init module_project5_init(void){
    return elv_register(&elevator_zfq);
}


static void __exit module_project5_exit(void){
    elv_unregister(&elevator_zfq);
}

module_init(module_project5_init);
module_exit(module_project5_exit);

