#include <linux/unistd.h>
#include<stdio.h>
#include<pwd.h>
#include<string.h>
#define MY_BUFFER_SIZE 10
#define __NR_my_syscall  548
//void get_info(int userid, void *buffer, void* task_ptr, int* isComplete){// isComplete is a boolean type to see if it finish traverse all the task in the queue.

int main(int argc, char* argv[])

{
	char username[100];
	//printf("%s", argv[1]);
	strcpy(username,argv[1]);


	struct passwd* pw = NULL;
	//while(1){

		//printf("Please input the user name\n");
		//scanf("%s", username);
		//printf("%s\n", username);
		pw = getpwnam(username);

		if(pw == NULL) {
			printf("The user name not existed\n");
		}
		else{
			int userid = pw->pw_uid;//1000;
			int isComplete = 0;
			typedef struct proc_info{
				int pid;
				char tty[64];
				long time;
				char comm[16];
			}proc_info;
			char  buffer[MY_BUFFER_SIZE* sizeof(proc_info)];
			int task_ptr = 0;
            int loop_iterator = 0;
			proc_info pinfo;
			//printf("%d\n", userid);
			printf("PID\tTTY\t\tTIME\tCOM\n");

			int len = MY_BUFFER_SIZE;
			while(isComplete == 0) {

                loop_iterator++;
               // printf("\n\n loop %d", loop_iterator);
                if(loop_iterator==1){
                    int start = 0;
                    syscall(__NR_my_syscall,userid,  buffer, &task_ptr,  &isComplete, &start);
                    len = start;
                  //  printf("first %d\n", start);
                }else {
                   // printf("sec len first %d\n", len);
                    task_ptr--;
                    syscall(__NR_my_syscall, userid, buffer, &task_ptr, &isComplete, &len);
                   // printf("sec len sec %d\n", len);
                }
				//printf("\n\n len %d", len);

				int i = 0;
                //printf("len%d\n",len );
                proc_info* bufferptr = (proc_info*)buffer;

                for (; i < len; i++) {
					memcpy(&pinfo, bufferptr, sizeof(proc_info));
					bufferptr++;
					//if(strcmp(pinfo.tty,""))
					//	strcpy(pinfo.tty,"?");
                    int h = pinfo.time/3600;
                    int m = (pinfo.time-3600*h)/60;
                    int s = pinfo.time-3600*h -60*m;

                    printf("%d\t%s\t\t%d:%d:%d\t%s\n", pinfo.pid, pinfo.tty, h,m,s, pinfo.comm);
				}
			}
			//printf("%d\n",syscall(__NR_my_syscall,10));
		}

	//}

	return 0;
}
