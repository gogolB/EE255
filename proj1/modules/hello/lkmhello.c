#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Souradeep Bhattacharya and Jesse Garcia");
MODULE_DESCRIPTION("A simple module that prints hello world from kernel space");
MODULE_VERSION("0.1");



static int __init lkmhello_init(void)
{
	printk(KERN_INFO,"Hello world! team02 in kernel space");
	return 0;
}


static void __exit lkmhello_exit(void){
	printk(KERN_INFO, "Goodbye world! team02 in kernel space");
}

module_init(lkmhello_init);
module_exit(lkmhello_exit);
