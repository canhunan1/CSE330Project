#include<linux/syscalls.h>
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/sched.h>    /* Needed for for_each_procee(), signal_struck and task_struck*/
#include <linux/tty.h>      /*Needed for tty_struct*/
#include <linux/jiffies.h>  /*utime, stime and HZ*/
#include <linux/cred.h>     /*Needed for cred (to get the uid for the user*/
#include <linux/string.h>
#define MY_BUFFER_SIZE 10

void get_info(int _userid, void *buffer, int* _iterator, int* _isComplete, int* _len);

asmlinkage long sys_my_syscall(int userid, void *buffer, int* _iterator, int* isComplete, int* len)
{
	printk(KERN_INFO "This is the new system call Jianan Yang implemented\n");

    get_info( userid,  buffer, _iterator,  isComplete, len);

	return 10;

}

void get_info(int _userid, void *buffer, int* _iterator, int* _isComplete, int* _len){// isComplete is a boolean type to see if it finish traverse all the task in the queue.
	int len=0;
	struct task_struct *task_list = NULL;
    //printk(KERN_INFO " sizeof(struct task_struct *) is %d, sizeof(int) is %d", sizeof(struct task_struct *),sizeof( int));
    /*if (copy_from_user(&task_list, task_ptr, sizeof(struct task_struct *))){
        printk(KERN_INFO "Error: get task_list \n");)
    };*/
   // printk(KERN_INFO "111 task_ptr is %d,,task_list is %d", *(int*)task_ptr,task_list);

    //struct tty_struct *tty;
	char* terminal_name;
	long mytime ;
    int userid = _userid;
    int isComplete = 0;
    //if(get_user(&userid, &_userid)){
    //    printk(KERN_INFO "Error: get userid");
    //};
    if(copy_from_user(&isComplete, _isComplete, sizeof(int))){
        printk(KERN_INFO " Error: get isComplete");
    }
	typedef struct proc_info{
		int pid;
		char tty[64];
		long time;
		char comm[16];
	}proc_info;

	printk(KERN_INFO "new message");
	proc_info pinfo;

	//FILE * file= fopen("output", "wb");
    printk(KERN_INFO "kernel buffer");

    char kernel_buffer[MY_BUFFER_SIZE* sizeof(proc_info)];

    printk(KERN_INFO "bufferptr");

    char* bufferptr = kernel_buffer;

    printk(KERN_INFO "task_list");

    /*if(task_list == NULL) {
        printk(KERN_INFO "task_list = &init_task");

        task_list = &init_task;
    }*/
	//int i = 0;
    int start = 0;
    if(copy_from_user( &start, _iterator, sizeof(int))){
        printk(KERN_INFO "Error: put len\n");
    };
    printk(KERN_INFO "the start is  %d\n", start);

    int iterator = 0;

	for_each_process(task_list){
		mytime = (long) (task_list->utime+task_list->stime)/HZ;
        printk(KERN_INFO "the iterator is %d\n", iterator);

		if (task_list->cred->uid.val == userid && iterator>=start) {

            //terminal_name = task_list->signal->tty->name;

            pinfo.pid = task_list->pid;

			printk(KERN_INFO "strcpy1");
			if(task_list->signal->tty != NULL){
                printk(KERN_INFO "strcpy1 not null");

				strncpy(pinfo.tty, task_list->signal->tty->name,64);

			}else{
                printk(KERN_INFO "strcpy1 null");

                strcpy(pinfo.tty, "?");
			}

			//strncpy(pinfo.tty,terminal_name,64);
			//else
			//   strncpy(pinfo.tty,"", 64);

			pinfo.time = mytime;


			printk(KERN_INFO "strcpy2");

			if(task_list->comm != NULL ){
                printk(KERN_INFO "strcpy2 not null");

                strncpy(pinfo.comm, task_list->comm,16);
            }else{
                printk(KERN_INFO "strcpy2 null");

                strcpy(pinfo.comm, "?");
            }
			/*if (file != NULL)
                fwrite(pinfo, sizeof(proc_info ), 1, file);
            printk("\n comm : %s terminal: %s pid : %d userid: %d time : %ld\n", task_list->comm, terminal_name,
                   task_list->pid, task_list->cred->uid.val, mytime);*/
			if(len<MY_BUFFER_SIZE){
                printk(KERN_INFO "write bufferptr");

				memcpy(bufferptr, &pinfo, sizeof(proc_info));

                printk(KERN_INFO "bufferptr++");

                bufferptr += sizeof(proc_info);

                len++;
			}else{
                iterator++;

                break;
            }

		}
        iterator++;
	}
    printk(KERN_INFO "length is %d\n", len);
    if(copy_to_user(_len, &len, sizeof(int))){
        printk(KERN_INFO "Error: put len\n");
    };
    printk(KERN_INFO "_iterator is %d\n", iterator);
    if(copy_to_user(_iterator, &iterator, sizeof(int))){
        printk(KERN_INFO "Error: put len\n");
    };

    //len = 0;

    printk(KERN_INFO "iscomplete");
    if(task_list == &init_task){
        isComplete = 1;
        printk(KERN_INFO "isComplete %d", isComplete);

  }

    if(copy_to_user(_isComplete, &isComplete, sizeof(int))){
        printk(KERN_INFO "Error: put isComplete\n");
    };
    //task_ptr = task_list;
    /*if(copy_to_user(&task_ptr,&task_list,sizeof(struct task_struct *))){
        printk(KERN_INFO "Error: put task_list\n");
    };
    printk(KERN_INFO "222 task_ptr is %d,,task_list is %d", *(int*)task_ptr,task_list);*/


    /*for_each_process(task_list) {

        terminal_name = task_list->signal->tty->name;
        mytime = (long) (task_list->utime+task_list->stime)/HZ;

        if (task_list->cred->uid.val == userid) {
            pinfo.userid = task_list->pid;
            strcpy(pinfo.tty,terminal_name);
            pinfo.time = mytime;
            strcpy(pinfo.comm, task_list->comm);
            if (file != NULL)
                fwrite(pinfo, sizeof(proc_info ), 1, file);
            printk("\n comm : %s terminal: %s pid : %d userid: %d time : %ld\n", task_list->comm, terminal_name,
                   task_list->pid, task_list->cred->uid.val, mytime);
            if(len<10){
                bufferptr += sizeof(proc_info);
                memcpy(bufferptr, &pinfo, sizeof(proc_info));
            }

            len++;
        }
    }*/

	if(copy_to_user( buffer, kernel_buffer, MY_BUFFER_SIZE* sizeof(proc_info))){
		printk(KERN_INFO "Error: copy to user error\n");

	};

	//  fclose(file);

}
