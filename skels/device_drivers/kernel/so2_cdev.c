/*
 * Character device drivers lab
 *
 * All tasks
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/xarray.h>


#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL"); /* Needed in order to avoid complains when the module is loaded. */

#define LOG_LEVEL	KERN_INFO

#define MY_MAJOR		42
#define MY_MINOR		0

#define NUM_MINORS		1
#define MODULE_NAME		"so2_cdev"
#define MESSAGE			"I'm inside the device\n"
#define IOCTL_MESSAGE		"Hello ioctl"







#ifndef BUFSIZ
#define BUFSIZ		4096

#endif
static void testerMethod(void){
	char a[] = "tester";
	void *p = &a;
	gfp_t gfp;
	struct xarray array;
	xa_init(&array);
	unsigned long index = 0;
	xa_store(&array,index,p,gfp);
	char *returnedValue = xa_load(&array,index);
	bool isEmpty = xa_empty(&array);
	char message[]= "failed";
	
	printk("%s\n" ,message );
	printk("%d\n", isEmpty);
	printk("%s\n", returnedValue);

} 



struct so2_device_data {
	/* TODO 2: add cdev member */
	struct cdev cdev; /* char device structure */
	/* TODO 4: add buffer with BUFSIZ elements */
	char buffer[BUFSIZ];
	size_t size;
	/* TODO 7: extra members for home */
	/* TODO 3: add atomic_t access variable to keep track if file is opened */
	atomic_t access;
}; /* Structure that holds the data of my device */

struct so2_device_data devs[NUM_MINORS];

static int so2_cdev_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data;

	/* TODO 2: print message when the device file is open. */
	printk(LOG_LEVEL "open device\n");
	/* TODO 3: inode->i_cdev contains our cdev struct, use container_of to obtain a pointer to so2_device_data */
	data = container_of(inode->i_cdev, struct so2_device_data, cdev); /* Macro that returns a pointer to my desired structure */
	file->private_data = data; /* For easier access in the future , not relevant atm */ 

	/* TODO 3: return immediately if access is != 0, use atomic_cmpxchg */
	if (atomic_cmpxchg(&data->access, 0, 1) != 0)
		return -EBUSY;

	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(10);	
	
	return 0;
}

static int
so2_cdev_release(struct inode *inode, struct file *file)
{
	/* TODO 2: print message when the device file is closed. */
	printk(LOG_LEVEL "closed device\n");

#ifndef EXTRA
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;

	/* TODO 3: reset access variable to 0, use atomic_set */
	atomic_set(&data->access, 0);
#endif
	return 0;
	
}

static ssize_t
so2_cdev_read(struct file *file,
		char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	size_t to_read;

#ifdef EXTRA
	/* TODO 7: extra tasks for home */
#endif

	/* TODO 4: Copy data->buffer to user_buffer, use copy_to_user */
	to_read = (size > data->size - *offset) ? (data->size - *offset) : size;
	if (copy_to_user(user_buffer, data->buffer + *offset, to_read) != 0)
		return -EFAULT;
	*offset += to_read;
	return to_read;
}

static ssize_t
so2_cdev_write(struct file *file,
		const char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
		printk("%lld Offset before \n", *offset);
		printk("%d Data size before \n", data->size);
char *temp_buffer[BUFSIZ];
	
	size = (*offset + size > BUFSIZ) ? (BUFSIZ - *offset) : size;
	/*if (copy_from_user(data->buffer + *offset, user_buffer, size) != 0)
		return -EFAULT;*/
	//copy_from_user(data->buffer + *offset, user_buffer, size);
	//copy_from_user(temp_buffer + *offset, user_buffer, size);
	//gfp_t GFP_KERNEL;
	
	struct xarray array;
	xa_init(&array);	
	
	printk("%d Sizes before \n", size);
	copy_from_user(data->buffer + *offset, user_buffer, size);
	copy_from_user(temp_buffer + *offset, user_buffer, size);
	//printk(" %s : VALOR ANTES DE ARRAY \n" ,data->buffer + *offset);
	//char *p = data->buffer;
	//printk("%s: VALOR DESPUES DE ARRAY \n" ,p );
	char *beforeValue = data->buffer + *offset;
	
	xa_store(&array,*offset,data->buffer,GFP_KERNEL);
	char *returnedValue = xa_load(&array,*offset);
	bool isEmpty = xa_empty(&array);
	
	*offset += size;
	data->size = *offset;
	printk("Beforep value:%s\n", beforeValue);
	printk("Returned value:%s\n", returnedValue);
	printk("%lld Offset after \n", *offset);
	printk("%d Data size after \n", data->size);
	return size;
}

static loff_t so2_cdev_lseek(struct file *file, loff_t offset, int orig){
	{
        loff_t new_pos = 0;
        printk("lseek function in work\n");
        switch (orig) {
        case 0 :        /*seek set*/
                new_pos = offset;
                break;
        case 1 :        /*seek cur*/
                new_pos = file->f_pos + offset;
                break;
        case 2 :        /*seek end*/
                new_pos = BUFFER_SIZE - offset;
                break;
        }
        if (new_pos > BUFFER_SIZE)
                new_pos = BUFFER_SIZE;
        if (new_pos < 0)
                new_pos = 0;
        file->f_pos = new_pos;
        return new_pos;
}
}

static long
so2_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	int ret = 0;
	int remains;

	switch (cmd) {
	/* TODO 6: if cmd = MY_IOCTL_PRINT, display IOCTL_MESSAGE */
	case MY_IOCTL_PRINT:
		printk(LOG_LEVEL "%s\n", IOCTL_MESSAGE);
		break;
	/* TODO 7: extra tasks, for home */
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations so2_fops = {
	.owner = THIS_MODULE, /* Just a pointer to the module that owns the structure. */

/* TODO 2: add open and release functions */
	.open = so2_cdev_open,
	.release = so2_cdev_release,
/* TODO 4: add read function */
	.read = so2_cdev_read,
/* TODO 5: add write function */
	.write = so2_cdev_write,
/* TODO 6: add ioctl function */
	.unlocked_ioctl = so2_cdev_ioctl,
	.llseek = so2_cdev_lseek
};

static int so2_cdev_init(void)
{
	int err;
	int i;
	testerMethod();


	/* TODO 1: register char device region for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR),
			NUM_MINORS, MODULE_NAME);
	if (err != 0) {
		pr_info("register chrdevice region");
		return err;
	}


	for (i = 0; i < NUM_MINORS; i++) {
#ifdef EXTRA
		/* TODO 7: extra tasks, for home */
#else
		/*TODO 4: initialize buffer with MESSAGE string */
		/*memcpy(devs[i].buffer, MESSAGE, sizeof(MESSAGE));
		devs[i].size = sizeof(MESSAGE);*/
#endif
		/* TODO 7: extra tasks for home */
		/* TODO 3: set access variable to 0, use atomic_set */
		atomic_set(&devs[i].access, 0);
		/* TODO 2: init and add cdev to kernel core */
		cdev_init(&devs[i].cdev, &so2_fops); /* embed my device-specific structure to the cdev struc. */
		cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1); /* tell the kernel about it. */
	}

	return 0;
}

static void so2_cdev_exit(void)
{
	int i;

	for (i = 0; i < NUM_MINORS; i++) {
		/* TODO 2: delete cdev from kernel core */
		cdev_del(&devs[i].cdev);
	}

	/* TODO 1 unregister char device region, for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
	

}

module_init(so2_cdev_init); /* Special Kernel Macros */
module_exit(so2_cdev_exit); /* Special Kernel Macros */

/* mknod /dev/so2_cdev c 42 0 */ 

