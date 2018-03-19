#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/sysfs.h>

#include <linux/module.h> 
#include <linux/printk.h> 
#include <linux/kobject.h> 
#include <linux/sysfs.h> 
#include <linux/init.h> 
#include <linux/fs.h> 
#include <linux/string.h> 

#include "../proj4/kernel/sys_rsv.h"
// Help
// pradheepshrinivasan.github.io
// cs.swarthmore.edu

#define DEVICE_NAME "partdev"
#define CLASS_NAME "part"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jesse Garcia and Souradeep Bhattacharya");
MODULE_DESCRIPTION("This module, when read will allow partitioning heuristics to be read and applied to the current CPU");
MODULE_VERSION("0.1");

static struct device_attribute mydev;
static struct kobject *my_kobj;

extern int rsv_getPolicy(void);
extern int rsv_setPolicy(int);

ssize_t mydev_do_read( struct device * dev ,struct device_attribute *attr, char * buf)
{
	int policy = rsv_getPolicy();
	switch(policy)
	{
		case BEST_FIT:
			buf[0] = 'B';
			buf[1] = 'F';
			buf[2] = '\n';
			break;
		case WORST_FIT:
			buf[0] = 'W';
			buf[1] = 'F';
			buf[2] = '\n';
			break;
		case FIRST_FIT:
			buf[0] = 'F';
			buf[1] = 'F';
			buf[2] = '\n';
			break;
		default:
			buf[0] = 'N';
			buf[1] = 'A';
			buf[2] = '\n';
			break;
	}
	printk(KERN_INFO"[PART] Policy is current: %s\n",buf);
	//return 2;
	return 3;
}

ssize_t mydev_do_write(struct device * dev, struct device_attribute * attr, const char* buf, size_t count)
{
	printk(KERN_INFO"[PART] Setting new Policy to %s\n", buf);
	if(strcmp(buf, "BF\n") == 0)
	{
		rsv_setPolicy(BEST_FIT);
	}
	else if(strcmp(buf, "WF\n") == 0)
	{
		rsv_setPolicy(WORST_FIT);
	}
	else if(strcmp(buf, "FF\n") == 0)
	{
		rsv_setPolicy(FIRST_FIT);
	}

	return count;
}

static int __init partdev_init(void) 
{ 

	mydev.attr.name = "partition_policy";
	mydev.attr.mode = 0664;
	mydev.show = mydev_do_read;
	mydev.store = mydev_do_write;

	printk(KERN_INFO"[PART] Module Creating Directories...\n");
	
	my_kobj = kobject_create_and_add("ee255",NULL);
	if(!my_kobj)
	{
		return -ENOMEM;
	}
	
	if(PTR_ERR(sysfs_create_file(my_kobj, &mydev.attr)))
	{
		return -ENOMEM;
	}

	printk(KERN_INFO"[PART] Module initialized\n");
	return 0;
}

static void __exit partdev_exit(void)
{
	if(my_kobj != NULL)
	{
		printk(KERN_INFO"[PART] Module removing directories...\n");
		sysfs_remove_file(my_kobj, &mydev.attr);
		kobject_put(my_kobj);
		my_kobj = NULL;
		
	}
	printk(KERN_INFO"[PART] LKM Removed\n");

}

module_init(partdev_init);
module_exit(partdev_exit);


