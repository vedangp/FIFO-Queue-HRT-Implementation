#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/param.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/string.h>

#include "HRT.h"

#define DEVICE_NAME_QUEUE	"SQUEUE"
#define SQUEUE_DEVICE1		"Squeue1"
#define SQUEUE_DEVICE2		"Squeue2"
#define SQUEUE_DEVICE1_MINOR	    0
#define SQUEUE_DEVICE2_MINOR	    1
#define QUEUE_LENGTH 10

int token = 0;

/* the queue element */ 
struct Queue_elem {
	int token_id;
	int time_stamp1;
	int time_stamp2;
	int time_stamp3;
	int time_stamp4;
	char string[81];
};

struct Squeue_dev {
	struct cdev cdev;
	char name[20];
	struct Queue_elem** squeue;
	int queue_head;
	int queue_tail;
	spinlock_t queue_lock;
} *squeue_devp1, *squeue_devp2;

static dev_t my_dev_number;      /* Allotted device number */
struct class *my_dev_class;          /* Tie with the device model */

/*
 * Helper functions 
 */

/*
 *  Queue Empty
 *  will return 1 if queue is empty else will return 0
 */
int queue_empty(struct Queue_elem** squeue, int head, int tail)
{

	if (tail == head)
		return 1;
	else 
		return 0;
}

/*
 *  Queue Full
 *  will return 1 if queue is full else will return 0
 */
int queue_full(struct Queue_elem** squeue, int head, int tail)
{
	if ((head + 1) % QUEUE_LENGTH == tail)
		return 1;
	else 
		return 0;
}

/*
 *  Enqueue squeue
 *  will return 0 for success and -1 for failure
 */
int enqueue(struct Squeue_dev* squeue_devp,struct Queue_elem* queue_elem)
{
	int head;
	int tail;    
	spin_lock(&(squeue_devp->queue_lock));
	head = squeue_devp->queue_head;
	tail = squeue_devp->queue_tail;    
	/* checks whether the queue is full, if not enqueues the element
	   else returns -1 */
	if (!queue_full(squeue_devp->squeue, head, tail))
	{	
		queue_elem->time_stamp2 = get_hrt_time_stamp();	
		queue_elem->token_id = token;
		token++;
		squeue_devp->squeue[head] = queue_elem;
		squeue_devp->queue_head = (squeue_devp->queue_head + 1)%QUEUE_LENGTH;
		spin_unlock(&(squeue_devp->queue_lock));
	}else 
	{
		spin_unlock(&(squeue_devp->queue_lock));
		return -1;
	}
	return 0;
}

/*
 * Dequeue squeue
 * will return 0 for success and -1 for failure
 */
struct Queue_elem*  dequeue(struct Squeue_dev* squeue_devp)
{
	struct Queue_elem* elem;		
	int head;
	int tail;   	
	spin_lock(&(squeue_devp->queue_lock));
	head = squeue_devp->queue_head;
	tail = squeue_devp->queue_tail;   	
	if (!queue_empty(squeue_devp->squeue,head,tail))
	{
		(squeue_devp->squeue[tail])->time_stamp3 = get_hrt_time_stamp();
		elem = squeue_devp->squeue[tail];
		squeue_devp->squeue[tail] = 0;
		squeue_devp->queue_tail = (squeue_devp->queue_tail + 1)%QUEUE_LENGTH;
		spin_unlock(&(squeue_devp->queue_lock));
		return elem;
	}
	else
	{
		spin_unlock(&(squeue_devp->queue_lock));
		return NULL;
	}
	return 0;
}


/*
 * Open squeue driver
 */
int squeue_open(struct inode *inode, struct file *file)
{
	struct Squeue_dev* squeue_devp; 

	/* This will get squeue_devp1 or squeue_devp2 depending on the device opened */
	squeue_devp =  container_of(inode->i_cdev,struct Squeue_dev,cdev);
    	
	/* Embedding device specific structure in file struct */
	file->private_data = squeue_devp;

	printk("\n%s opened\n", squeue_devp->name);
	return 0;
}

/*
 * Release squeue driver
 */
int squeue_release(struct inode *inode, struct file *file)
{
	struct Squeue_dev* squeue_devp = file->private_data;
	printk("\n%s Closed.\n",squeue_devp->name);
	return 0;   
}
/*
 * Read to squeue driver
 */
ssize_t squeue_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
	int res;
	struct Squeue_dev* squeue_devp;
	struct Queue_elem* elem;	
	squeue_devp = file->private_data;
	elem = dequeue(squeue_devp);
	if (elem == NULL)
		return -1;    
	printk("%s head : %d tail : %d token : %d \n",squeue_devp->name, squeue_devp->queue_head, squeue_devp->queue_tail,elem->token_id);
	
	res = copy_to_user(buf,(char*)elem,sizeof(struct Queue_elem));
	if(res)
	{
		kfree(elem);
		return -EFAULT;
	}
	printk("before kfree for %d in %s \n",elem->token_id,squeue_devp->name);
	kfree(elem);
			
	return sizeof(struct Queue_elem);    
}

/*
 * Write to squeue driver
 */
ssize_t squeue_write(struct file *file, const char *buf,
           size_t count, loff_t *ppos)
{
	struct Squeue_dev* squeue_devp;
	struct Queue_elem* elem;
	squeue_devp = file->private_data;
	elem = kmalloc(sizeof(struct Queue_elem),GFP_KERNEL);
	
	if (elem == NULL)
	{
		kfree(elem);
		return -ENOMEM;
	}
	
	if (copy_from_user((char *)elem, buf, sizeof(struct Queue_elem)))
	{
		kfree(elem);
		return -EFAULT;
	}
	
	if(enqueue(squeue_devp,elem) == -1)
	{
		kfree(elem);
		return -1;
	}
	printk("%s head : %d tail : %d token : %d\n",squeue_devp->name, squeue_devp->queue_head, squeue_devp->queue_tail,elem->token_id);
	
	return (sizeof(struct Queue_elem));
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations My_fops = {
    .owner	= THIS_MODULE,		    /* Owner */
    .open	= squeue_open,		    /* Open method */
    .release	= squeue_release,	    /* Release method */
    .read	= squeue_read,		    /* Read method */
    .write	= squeue_write,		    /* Write method */
};

void init_squeue_dev_struct(struct Squeue_dev* squeue_devp, char* device_name)
{
	/* initialize head and tail to 0 */
	squeue_devp->queue_head = 0;
	squeue_devp->queue_tail = 0;	

	/* Allocate memory to both the queues */
	squeue_devp->squeue = kmalloc(sizeof(struct Queue_elem*) * QUEUE_LENGTH,GFP_KERNEL); 
	    
	/* Set memory to 0. */
	squeue_devp->squeue = memset(squeue_devp->squeue,0,sizeof(struct Queue_elem*) * QUEUE_LENGTH);	
    
	/* Request I/O region */
	sprintf(squeue_devp->name, device_name);  

	/* Initializing the spin lock. */
	spin_lock_init(&(squeue_devp->queue_lock));    

	/* Connect the file operations with the cdev */
	cdev_init(&squeue_devp->cdev, &My_fops);
	squeue_devp->cdev.owner = THIS_MODULE;

}

/*
 * Driver Initialization
 */
static int __init squeue_init(void)
{
	int ret;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&my_dev_number, 0, 2, DEVICE_NAME_QUEUE) < 0) {
		printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	squeue_devp1 = kmalloc(sizeof(struct Squeue_dev), GFP_KERNEL);
	squeue_devp2 = kmalloc(sizeof(struct Squeue_dev), GFP_KERNEL);
	/* Populate sysfs entries */
	my_dev_class = class_create(THIS_MODULE, DEVICE_NAME_QUEUE);

	init_squeue_dev_struct(squeue_devp1, SQUEUE_DEVICE1);
	init_squeue_dev_struct(squeue_devp2, SQUEUE_DEVICE2);
 
	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&squeue_devp1->cdev, (my_dev_number), 1);
	ret = cdev_add(&squeue_devp2->cdev, MKDEV(MAJOR(my_dev_number),(MINOR(my_dev_number)+1)), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	device_create(my_dev_class, NULL, MKDEV(MAJOR(my_dev_number), 0), NULL, SQUEUE_DEVICE1);		
	device_create(my_dev_class, NULL, MKDEV(MAJOR(my_dev_number), 1), NULL, SQUEUE_DEVICE2);		
	
	printk("Squeue Driver Initialized.\n");
	return 0;
}

/*
 * Driver Exit
 */
static void __exit squeue_exit(void)
{
	int i;
	/* Release the major number */
	unregister_chrdev_region((my_dev_number), 2);

	/* Destroy device */
	device_destroy (my_dev_class, MKDEV(MAJOR(my_dev_number), 0));
	device_destroy (my_dev_class, MKDEV(MAJOR(my_dev_number), 1));
	cdev_del(&squeue_devp1->cdev);
	cdev_del(&squeue_devp2->cdev);

	for (i=0;i<QUEUE_LENGTH;i++)
	{
		if(squeue_devp1->squeue[i] != 0)
			kfree(squeue_devp1->squeue[i]);	
		if(squeue_devp2->squeue[i] != 0)
			kfree(squeue_devp2->squeue[i]);
	}
    
	/* Freeing the queues */
	kfree(squeue_devp1->squeue);
	kfree(squeue_devp2->squeue);
    
	/* Freeing the device specific structs */
	kfree(squeue_devp1);
	kfree(squeue_devp2);

	/* Destroy driver_class */
	class_destroy(my_dev_class);

	printk("Squeue Driver removed.\n"); 
}

module_init(squeue_init);
module_exit(squeue_exit);
MODULE_LICENSE("GPL v2");

