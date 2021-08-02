/*
 * Character device drivers task
 *
 * 
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
#define MODULE_NAME		"so2_cdev"
#define DEVICENAME "so2_cdev"
#define BUFSIZ		4096
dev_t dev_num;          /* will hold the major number that the kernel gives*/
int major_number;   

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
	struct cdev cdev; /* char device structure */
	char buffer[BUFSIZ];
	size_t size;
	atomic_t access;
}; /* Structure that holds the data of my device */

struct so2_device_data devs;

static int so2_cdev_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data;

	printk(LOG_LEVEL "open device\n");
	data = container_of(inode->i_cdev, struct so2_device_data, cdev); /* Macro that returns a pointer to my desired structure */
	file->private_data = data; /* For easier access in the future , not relevant atm */ 

	/*  return immediately if access is != 0, use atomic_cmpxchg */
	if (atomic_cmpxchg(&data->access, 0, 1) != 0)
		return -EBUSY;

	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(10);	
	
	return 0;
}

static int
so2_cdev_release(struct inode *inode, struct file *file)
{
	printk(LOG_LEVEL "closed device\n");

#ifndef EXTRA
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;

	/*reset access variable to 0, use atomic_set */
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


static const struct file_operations so2_fops = {
	.owner = THIS_MODULE, /* Just a pointer to the module that owns the structure. */

	.open = so2_cdev_open,
	.release = so2_cdev_release,
	.read = so2_cdev_read,
	.write = so2_cdev_write,
	.llseek = so2_cdev_lseek
};

static int so2_cdev_init(void)
{
	int err;
	int i;
	testerMethod();

/*err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR),
			NUM_MINORS, MODULE_NAME); */
err = alloc_chrdev_region(&dev_num, 0, 1, DEVICENAME);
	if (err < 0) {
		pr_info("Can't allocate chrdevice region");
		printk("error");
		return err;
	}else{
		printk(KERN_INFO " charDev : mjor number allocated succesful\n");
        major_number = MAJOR(dev_num);
		  printk(KERN_INFO "charDev : major number of our device is %d\n", major_number);
        printk(KERN_INFO "charDev : to use mknod /dev/%s c %d 0\n", DEVICENAME, major_number);
		/*memcpy(devs[i].buffer, MESSAGE, sizeof(MESSAGE));
		devs[i].size = sizeof(MESSAGE);*/

		atomic_set(&devs.access, 0);
		cdev_init(&devs.cdev, &so2_fops); /* embed my device-specific structure to the cdev struc. */
		cdev_add(&devs.cdev, dev_num, 1); /* tell the kernel about it. */
	}

	

	return 0;
}

static void so2_cdev_exit(void)
{
	cdev_del(&devs.cdev);
	unregister_chrdev_region(dev_num, 1);
}

module_init(so2_cdev_init); /* Special Kernel Macros */
module_exit(so2_cdev_exit); /* Special Kernel Macros */



