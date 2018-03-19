#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/sched.h>

#include "sys_rsv.h"
// Help
// pradheepshrinivasan.github.io
// cs.swarthmore.edu

#define DEVICE_NAME "partdev"
#define CLASS_NAME "part"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jesse Garcia and Souradeep Bhattacharya");
MODULE_DESCRIPTION("This module, when read will allow partitioning", 
"heuristics to be read and applied to the current CPU");
MODULE_VERSION("0.1");


static struct class* partdevClass = NULL;
static struct device* partdevDevice = NULL;

// prototypes for functions that we'll use today
static int 	partdev_query(void);
static void 	partdev_specify(int newPolicy );

static struct file_operations fops = 
{
	.get = partdev_query,
	.set = partdev_specify
};

static int __init partdev_init(void) 
{ 
	pr_debug("Module initialized successfully \n");
	
	static struct kobject *my_kobj, *partpol;
	my_kobj = kobject_create_and_add("ee255",NULL);
	if(!my_kobj)
	{
		return -ENOMEM;
	}
	partpol = kobject_create_and_add("partition_policy",NULL);
	if(!partpol)
	{
		return -ENOMEM;
	}
	kobject_put(my_kobj);
	return 0;
}

static void __exit partdev_exit(void)
{
	printk(KERN_INFO"[PARTDEV] LKM Removed\n");
	pr_debug("Module exited successfully \n");
}

static int partdev_query(void)
{
	return rsv_getPolicy;
}

static void partdev_set(int newPolicy)
{
	rsv_setPolicy(newPolicy);
}
module_init(mymodule_init);
module_exit(mymodule_exit);


