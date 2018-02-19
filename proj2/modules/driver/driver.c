#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/sched.h>

#define DEVICE_NAME "rsvdev"
#define CLASS_NAME "rsv"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Souradeep Bhattacharya and Jesse Garcia");
MODULE_DESCRIPTION("This module, when read, will output a list of tasks having active reservations");
MODULE_VERSION("0.1");

static int majorNumber;
static int numberOpens = 0;

static struct class* rsvdevClass = NULL;
static struct device* rsvdevDevice = NULL;

static char message[1024] = {0};

// Prototype functions for the character driver -- must come before struct defination
static int 	dev_open(struct inode*, struct file *);
static int 	dev_release(struct inode*, struct file *);
static ssize_t  dev_read(struct file*, char *, size_t, loff_t*);

static struct file_operations fops = 
{
	.open = dev_open,
	.read = dev_read,
	.release = dev_release
};

static int __init rsvdev_init(void){
	printk(KERN_INFO "[RSVDEV] Initing rsvdevChar LKM\n");

	// Try to allocate a major number dynamically
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if(majorNumber < 0 )
	{
		printk(KERN_ALERT"[RSVDEV] Failed to get major number\n");
		return majorNumber;
	}
	printk(KERN_INFO"[RSVDEV] Registed with majornumber %d\n",majorNumber);

	// Register device class
	rsvdevClass = class_create(THIS_MODULE, CLASS_NAME);
	// Check for error
	if(IS_ERR(rsvdevClass))
	{
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT"[RSVDEV] Failed to create device class\n");
		return PTR_ERR(rsvdevClass);
	}
	printk(KERN_INFO"[RSVDEV] Device class was registered\n");

	rsvdevDevice = device_create(rsvdevClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if(IS_ERR(rsvdevDevice))
	{
		class_destroy(rsvdevClass);
		unregister_chrdev(majorNumber,  DEVICE_NAME);
		printk(KERN_ALERT"[RSVDEV] Failed to create the device\n");
		return PTR_ERR(rsvdevDevice);
	}
	printk(KERN_INFO"[RSVDEV] Device created successfully\n");

	return 0;
	
}

static void __exit rsvdev_exit(void)
{
	device_destroy(rsvdevClass, MKDEV(majorNumber,0));
	class_unregister(rsvdevClass);
	class_destroy(rsvdevClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO"[RSVDEV] LKM Removed\n");
}

static int dev_open(struct inode *inodep, struct file *fileptr)
{
	if(numberOpens >= 1)
	{
		printk(KERN_WARNING"[RSVDEV] Can not open because open already\n");
		return EBUSY;
	}
	
	numberOpens++;
	printk(KERN_INFO"[RSVDEV] Device has been opened\n");
	return 0;
}

static ssize_t dev_read(struct file *fileptr, char* buffer, size_t len, loff_t *offset)
{
	int errorCount =0;
	int location = snprintf(message, 1024, "pid\ttgid\tprio\tname\n");
	struct task_struct *g,*p;
	rcu_read_lock();
	do_each_thread(g,p)
	{
		if(p->rsv_task == 1)
		{
			location += snprintf(message + location, 1024 - location, "%d\t%d\t%d\t%s\n",p->pid, p->tgid, p->rt_priority, p->comm);
		}
	} while_each_thread(g,p);
	rcu_read_unlock();
	errorCount = copy_to_user(buffer, message + (*offset), location-(*offset));
	if(errorCount==0)
	{
		printk(KERN_INFO"[RSVDEV] Info copied to user(%d)(offset:%d)\n",location,(int)(*offset));	
		//(*offset) += location - (*offset);		
		
		//return location - (*offset);
		return simple_read_from_buffer(buffer, len, offset, message, location);
	}
	else
	{
		printk(KERN_WARNING"[RSVDEV] Failed to send %d bytes to user\n", errorCount);
		return -EFAULT;
	}
	
}

static int dev_release(struct inode *inodep, struct file *fileptr)
{
	if(numberOpens > 0)
	{
		printk(KERN_INFO"[RSVDEV] Device has been closed\n");
		numberOpens--;
		return 0;
	}
	else
	{
		printk(KERN_WARNING"[RSVDEV] Device was not open!\n");
		return -EFAULT;
	}
}

module_init(rsvdev_init);
module_exit(rsvdev_exit);
