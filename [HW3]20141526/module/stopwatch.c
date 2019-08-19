#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#define FND_ADDRESS 0x08000004
static int stopwatch_major = 242, stopwatch_minor = 0;
static dev_t stopwatch_dev;
static struct cdev stopwatch_cdev;
static unsigned char *fnd_addr;
struct timer_list timer;
struct timer_list end_timer;
static int seconds;
static int fndmin, fndsec;
static short fndminsec;
bool start_flag, end_flag;
bool pause_flag, timer_flag;
unsigned long cur_jiffies, pause_jiffies;

static int stopwatch_open(struct inode *, struct file *);
static int stopwatch_release(struct inode *, struct file *);
static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

static void timer_function(unsigned long);
static void end_timer_function(unsigned long none);
DECLARE_WAIT_QUEUE_HEAD(my_queue);
irqreturn_t home_handler(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t back_handler(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t volplus_handler(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t volminus_handler(int irq, void* dev_id, struct pt_regs* reg);

static struct file_operations stopwatch_fops =
{
    .open = stopwatch_open,
    .write = stopwatch_write,
    .release = stopwatch_release,
};


/*
 	FUNCTION NAME : timer_function
	PARAMETER : none
	TASK : INCREASE THE SECONDS VALUE AND UPDATE FND VALUE, AND ADD TIMER TO DO THIS FUNCTION RECURSIVELY
*/
static void timer_function(unsigned long none){
    /* do something */
    timer_flag = true;
    seconds += 1;
    fndmin = (seconds / 60) % 60;
    fndsec = seconds % 60;
    fndminsec = (fndmin / 10) << 12 | (fndmin % 10) << 8 | (fndsec / 10) << 4 | (fndsec % 10);
    outw(fndminsec, (unsigned int)fnd_addr);

    /* add timer */
    timer.expires = get_jiffies_64() + HZ;
    cur_jiffies = get_jiffies_64();
    timer.data = 0;
    timer.function = timer_function;
    add_timer(&timer);
}

/*
 	FUNCTION NAME : end_timer_function
	PARAMETER : none
	TASK : CHECK THE END_FLAG TO CHECK THAT THE VOL- BUTTON IS PUSHED 3SECONDS. IT MEANS end_flag == true. SO,
	IF end_flag IS true, INITIALIZE THE FND IN THE BOARD, AND WAKE UP THE PROCESS
*/
static void end_timer_function(unsigned long none){
    printk("end_flag value : %d\n", end_flag);
    if(end_flag == true){
        start_flag = false;
        outw(0, (unsigned int)fnd_addr);
        seconds = 0;
        __wake_up(&my_queue, 1, 1, NULL);
    }
}

/*
 	INTERRUPT HANDLER NAME : home_handler
	PARAMETER : int irq, void* dev_id, struct pt_regs* reg
	TASK : 	WHEN HOME BUTTON IN THE BOARD IS PUSHED, THIS HANDLER IS CALLED.
			THIS HANDLER ADD THE TIMER TO UPDATE THE FND(START)
*/
irqreturn_t home_handler(int irq, void* dev_id, struct pt_regs* reg){
    printk(KERN_ALERT "home_handler!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1,11)));
    //if(start_flag == false && pause_flag == false){
    if(start_flag == false){
        timer.expires = get_jiffies_64() + HZ;
        cur_jiffies = get_jiffies_64();
        timer.data = 0;
        timer.function = timer_function;
        add_timer(&timer);
        start_flag = true;
        pause_flag = false;
    }
    return IRQ_HANDLED;
}
/*
 	INTERRUPT HANDLER NAME : back_handler
	PARAMETER : int irq, void* dev_id, struct pt_regs* reg
	TASK : 	WHEN BACK BUTTON IN THE BOARD IS PUSHED, THIS HANDLER IS CALLED.
			THIS HANDLER STOP TO UPDATE THE FND VALUE, THAT IS, STOP THE STOPWATCH.
			WHEN STOPPING THE STOPWATCH, IT SAVE THE "ms"
*/
irqreturn_t back_handler(int irq, void* dev_id, struct pt_regs* reg){
    printk(KERN_ALERT "back_handler!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1,12)));
    if(pause_flag == false){
        /* WHEN PAUSE, SAVE THE "ms" */
        pause_jiffies = get_jiffies_64();
        del_timer(&timer);
        pause_flag = true;
    }
    else if(pause_flag == true && timer_flag == true){
        /* WHEN RESTART AFTER PAUSE, UPDATE THE TIME */
        timer.expires = get_jiffies_64() + (HZ - (pause_jiffies - cur_jiffies));
        printk("pause_jiffies - cur_jiffies : %lu\n", pause_jiffies - cur_jiffies);
        cur_jiffies = get_jiffies_64();
        timer.data = 0;
        timer.function = timer_function;
        add_timer(&timer);
        pause_flag = false;
    }
    return IRQ_HANDLED;
}
/*
 	INTERRUPT HANDLER NAME : volplus_handler
	PARAMETER : int irq, void* dev_id, struct pt_regs* reg
	TASK : 	WHEN VOL+ BUTTON IN THE BOARD IS PUSHED, THIS HANDLER IS CALLED.
			THIS HANDLER RESET THE FND DEVICE ON THE BOARD. SET THE FND VALUE 0000
*/
irqreturn_t volplus_handler(int irq, void* dev_id, struct pt_regs* reg){
    printk(KERN_ALERT "volplus_handler!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2,15)));
    seconds = 0;
    outw(0, (unsigned int)fnd_addr);
    if(pause_flag == false){
        del_timer(&timer);
        timer.expires = get_jiffies_64() + HZ;
        timer.data = 0;
        timer.function = timer_function;
        add_timer(&timer);
    }

    return IRQ_HANDLED;
}
/*
	INTERRUPT HANDLER NAME : volminus_handler
	PARAMETER : int irq, void* dev_id, struct pt_regs* reg
	TASK :	WHEN VOL- BUTTON IN THE BOARD IS PUSHED, THIS HANDLER IS CALLED.
			THIS HANDLER ADD END_TIMER TO CHECK END CONDITION. AND IF THE CONDITION
			IS SATISFIED, THEN IT WILL WAKE UP.
*/
irqreturn_t volminus_handler(int irq, void* dev_id, struct pt_regs* reg){
    printk(KERN_ALERT "volminus_handler!!! = %x\n", gpio_get_value(IMX_GPIO_NR(5,14)));
  
    /* WHEN VOL- BUTTON IS PUSHED */
    if(end_flag == false){
        end_flag = true;
        end_timer.expires = get_jiffies_64() + 3*HZ;
        end_timer.data = 0;
        end_timer.function = end_timer_function;
        add_timer(&end_timer);
    }
    /* WHEN VOL- BUTTON IS RELEASED */
    else{
        end_flag = false;
        del_timer(&end_timer);
    }
    return IRQ_HANDLED;
}

/*
 	FUNCTION NAME : stopwatch_open
	PARAMETER : struct inode *minode, struct file *mfile
	TASK : 	THIS FUNCTION REGISTER INTERRUPT NUMBER IRQ AND INTERRUPT HANDLER
			BY USING THE FUNCTION "REQUEST_IRQ" AND
			INITIALIZE THE TIMERS, FND_ADDRESS, AND THE FLAGS
*/
static int stopwatch_open(struct inode *minode, struct file *mfile){
    int ret;
    int irq;

    printk(KERN_ALERT "Open Module\n");
    
    /* INT 1 HOME */
    gpio_direction_input(IMX_GPIO_NR(1,11));
    irq = gpio_to_irq(IMX_GPIO_NR(1,11));
    printk(KERN_ALERT "IRQ Number : %d\n", irq);
    ret = request_irq(irq, home_handler, IRQF_TRIGGER_FALLING, "HOME", 0);

    /* INT 2 BACK */
    gpio_direction_input(IMX_GPIO_NR(1,12));
    irq = gpio_to_irq(IMX_GPIO_NR(1,12));
    printk(KERN_ALERT "IRQ Number : %d\n", irq);
    ret = request_irq(irq, back_handler, IRQF_TRIGGER_FALLING, "BACK", 0);

    /* INT 3 VOL+ */
    gpio_direction_input(IMX_GPIO_NR(2,15));
    irq = gpio_to_irq(IMX_GPIO_NR(2,15));
    printk(KERN_ALERT "IRQ Number : %d\n", irq);
    ret = request_irq(irq, volplus_handler, IRQF_TRIGGER_FALLING, "VOL+", 0);
    
    /* INT 4 VOL- */
    gpio_direction_input(IMX_GPIO_NR(5,14));
    irq = gpio_to_irq(IMX_GPIO_NR(5,14));
    printk(KERN_ALERT "IRQ Number : %d\n", irq);
    ret = request_irq(irq, volminus_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "VOL-", 0);
    
    fnd_addr = ioremap(FND_ADDRESS, 0x04);
    init_timer(&timer);
    init_timer(&end_timer);
    seconds = 0;
    start_flag = false;
    end_flag = false;
    pause_flag = false;
    timer_flag = false;
    
    return 0;
}
/*
 	FUNCTION NAME : stopwatch_release
	PARAMETER : struct inode *minode, struct file *mfile
	TASK : FREE IRQS WHEN THIS MODULE IS RELEASED
*/
static int stopwatch_release(struct inode *minode, struct file *mfile){
    free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
    free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
    free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
    free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
    del_timer(&timer);
    del_timer(&end_timer);

    printk(KERN_ALERT "Release Module\n");
    return 0;
}
/*
 	FUNCTION NAME : stopwatch_write
	PARAMETER : struct file *filp, const char *buf, size_t count, loff_t *f_pos
	TASK : THIS FUNCTION WILL BE CALLED BY USER PROGRAM. THIS FUNCTION CALL SLEEP FUNCTION AND WAIT THE END CONDITION.
*/
static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
    printk("IN STOPWATCH_WRITE FUNCTON, SLEEP...\n");
    interruptible_sleep_on(&my_queue);
    printk("WAKE UP!\n");
    return 0;
}

/*
 	FUNCTION NAME : stopwatch_register_cdev
	PARAMETER : void
	TASK : REGISTER CHARACTER DEVICE WHICH HAS MAJOR NUMBER 242(stopwatch_major)
*/
static int stopwatch_register_cdev(void){
    int error;
    if(stopwatch_major){
        stopwatch_dev = MKDEV(stopwatch_major, stopwatch_minor);
        error = register_chrdev_region(stopwatch_dev, 1, "stopwatch");
    }
    else{
        error = alloc_chrdev_region(&stopwatch_dev, stopwatch_minor, 1, "stopwatch");
        stopwatch_major = MAJOR(stopwatch_dev);
    }
    if(error < 0){
        printk(KERN_WARNING "stopwatch: can't get major %d\n", stopwatch_major);
        return error;
    }
    printk(KERN_ALERT "major number = %d\n", stopwatch_major);
    cdev_init(&stopwatch_cdev, &stopwatch_fops);
    stopwatch_cdev.owner = THIS_MODULE;
    stopwatch_cdev.ops = &stopwatch_fops;
    error = cdev_add(&stopwatch_cdev, stopwatch_dev, 1);
    if(error){
        printk(KERN_NOTICE "inter Register Error %d\n", error);
    }
    return 0;
}

/*
 	FUNCTION NAME : stopwatch_init
	PARAMETER : void
	TASK : 	WHEN INSMOD THIS MODULE, THIS FUNCTION WILL BE CALLED.
			PRINT MESSAGES IF THIS MODULE BE INSMOD SUCCESSLY
*/
static int __init stopwatch_init(void){
    int result;
    if((result = stopwatch_register_cdev()) < 0){
        return result;
    }
    printk(KERN_ALERT "Init Module Success \n");
    printk(KERN_ALERT "Device : /dev/stopwatch, Major Num : %d \n", stopwatch_major);
    return 0;
}

/*
 	FUNCTION NAME: stopwatch_exit
	PARAMETER : void
	TASK : WHEN THIS MODULE IS RELEASED BY RMMOD, THIS FUNCTION WILL BE CALLED.
*/
static void __exit stopwatch_exit(void){
    cdev_del(&stopwatch_cdev);
    unregister_chrdev_region(stopwatch_dev, 1);
    iounmap(fnd_addr);
    //del_timer(&timer);
    //del_timer(&end_timer);
    printk(KERN_ALERT "Remove Module Success \n");
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);
MODULE_LICENSE("GPL");
