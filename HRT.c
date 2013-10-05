#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/param.h>
#include <asm/uaccess.h>
#include <linux/pci.h>

#define PRESCALAR 0

#include "HRT.h"

static dev_t my_dev_number;      /* Allotted device number */
struct class *my_dev_class;          /* Tie with the device model */

/*
 * Driver Open
 */
int hrt_open(struct inode *inode, struct file *file)
{
	struct HRT_dev *hrt_devp;
	struct omap_dm_timer* timer;
	
	/* find the location for hrt_devp */
	hrt_devp = container_of(inode->i_cdev, struct HRT_dev, cdev);
	
	/* make it accessible to other calls */
	file->private_data = hrt_devp;
	
	/* Request a timer */
	hrt_devp->timer = omap_dm_timer_request();	
	
	timer = hrt_devp->timer;
	
	/* set prescalar */
	if(!omap_dm_timer_set_prescaler(timer, PRESCALAR))
	{
	    printk("prescaler not set.\n");
	}
	/* enable the timer */	
	omap_dm_timer_enable(timer);
	
	printk("\n%s opened\n", hrt_devp->name);
	return 0;
}

/*
 * Release My driver
 */
int hrt_release(struct inode *inode, struct file *file)
{
	struct HRT_dev *hrt_devp = file->private_data;
	struct omap_dm_timer *timer = hrt_devp->timer;
	omap_dm_timer_disable(timer);
	omap_dm_timer_free(timer);
	printk("\n%s closed.\n",hrt_devp->name);
	return 0;
}

/*
 * Read to g driver
 */
ssize_t hrt_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
	int res;
	unsigned int timer_value;
	struct HRT_dev* hrt_devp;
	
	/* Accessing the device specific struct stored in file->private_data */
	hrt_devp = file->private_data;
    
	/* get the current value of the timer */
	timer_value = omap_dm_timer_read_counter(hrt_devp->timer);
	
	/* check whether the user space buffer is writable */
	if(!access_ok(VERIFY_WRITE,buf,sizeof(unsigned int)))
		return -EPERM;	

	/* copying data to user space buffer. */
	if(res = __put_user(timer_value, buf))
	{
		return -EFAULT;
	}
	
	return res;
}

long hrt_ioctl(struct file* file,unsigned int cmd, unsigned long arg)
{
	struct HRT_dev* hrt_devp;
	struct omap_dm_timer* timer;
	/* Accessing the device specific struct stored in file->private_data */
	hrt_devp = file->private_data;
	timer = hrt_devp->timer;

	switch(cmd)
	{
		case CMD_START_TIMER:
			if (!omap_dm_timer_start(timer))
			{
				return -EINVAL;
			}
			break;
		case CMD_STOP_TIMER:
			if (!omap_dm_timer_stop(timer))
			{
				return -EINVAL;
			}
			break;
		case CMD_SET_TIMER_COUNTER_VALUE:
			if(!omap_dm_timer_write_counter(timer,(int)arg))
			{
			    return -EINVAL;
			}
			break;
		default:
			return -ENOTTY;
			break;
	}
	return 0;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations My_fops = {
    .owner	    = THIS_MODULE,          /* Owner */
    .open	    = hrt_open,		    /* Open method */
    .release	    = hrt_release,	    /* Release method */
    .read	    = hrt_read,		    /* Read method */
    .unlocked_ioctl = hrt_ioctl,	    /* IOCTL method */
};
/*
 * Driver Initialization
 */
static int __init hello_init(void)
{
	int ret;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&my_dev_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	my_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	hrt_devp = kmalloc(sizeof(struct HRT_dev), GFP_KERNEL);

	
	/* Request I/O region */
	sprintf(hrt_devp->name, DEVICE_NAME); 

	/* Connect the file operations with the cdev */
	cdev_init(&hrt_devp->cdev, &My_fops);
	hrt_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&hrt_devp->cdev, (my_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	device_create(my_dev_class, NULL, MKDEV(MAJOR(my_dev_number), 0), NULL, DEVICE_NAME);		
	
	printk("HRT Driver Initialized.\n");
	return 0;
}

/*
 * Driver Exit
 */
static void __exit hello_exit(void)
{
	
	/* Release the major number */
	unregister_chrdev_region((my_dev_number), 1);

	/* Destroy device */
	device_destroy (my_dev_class, MKDEV(MAJOR(my_dev_number), 0));
	cdev_del(&hrt_devp->cdev);
	kfree(hrt_devp);
	
	/* Destroy driver_class */
	class_destroy(my_dev_class);

	printk("My Driver removed.\n"); 
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL v2");
