/*
 *  hello-1.c - The simplest kernel module.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/pid.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/mm.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JIANAN YANG");

static int pid;
module_param(pid, int, S_IRUSR | S_IWUSR );

static int __init module_project4_init(void)
{

    printk(KERN_INFO "pid: %d\n", pid);
    struct task_struct* task = pid_task(find_vpid(pid), PIDTYPE_PID );
    if(task == NULL) {
        printk(KERN_INFO
        "NULL task\n");
        return 0;
    }
    struct mm_struct* mm = task->mm;
    int counter_exist = 0;
    int counter_swap = 0;
    down_read(&mm->mmap_sem);

    struct vm_area_struct *mmap = mm->mmap;
    pgd_t* pgd;
    pmd_t* pmd;
    pud_t* pud;
    pte_t * ptep, pte;
    
    while(mmap != NULL) {
        unsigned long address = mmap->vm_start;
        unsigned long end_address = mmap->vm_end;
        if(stack_guard_page_start(mmap,address))
            address += PAGE_SIZE;
        if(stack_guard_page_start(mmap,end_address))
            end_address -= PAGE_SIZE;
        for(;address<end_address; address += PAGE_SIZE) {

           //printk(KERN_INFO "task start address: %lu\n", address);
            pgd = pgd_offset(mm, address);
            if (pgd_none(*pgd) || pgd_bad(*pgd)) {
                printk(KERN_INFO "pgd error");
                break;
            }
            pud = pud_offset(pgd, address);
            if (pud_none(*pud) || pud_bad(*pud)) {
                printk(KERN_INFO "pud error");
                break;
            }
            pmd = pmd_offset(pud, address);
            if (pmd_none(*pmd) || pmd_bad(*pmd)) {
                printk(KERN_INFO "pmd error");
                break;
            }
            ptep = pte_offset_map(pmd, address);
            if (!ptep) {
                printk(KERN_INFO "ptep null");
                break;
            }
            pte = *ptep;
            if (pte_none(pte)) {
                //not exist
                //printk(KERN_INFO "not exist\n");
            } else if (pte_present(pte)) {
               // printk(KERN_INFO "exist\n");
                counter_exist++;
            } else {
               // printk(KERN_INFO "swap\n");
                counter_swap++;
            }
        }
        mmap = mmap->vm_next;
    }
    up_read(&mm->mmap_sem);
    
    printk(KERN_INFO "residency pages: %d, swap pages: %d\n", counter_exist, counter_swap);

    return 0;
}





static void __exit module_project4_exit(void)
{
    printk(KERN_INFO "Goodbye world 1.\n");
}

module_init(module_project4_init);
module_exit(module_project4_exit);

