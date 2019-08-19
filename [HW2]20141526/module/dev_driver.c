#include "dev_driver.h"

struct file_operations dev_fops =
{
    .open           = dev_open,
    //.write          = dev_write,
    .release        = dev_release,
    .unlocked_ioctl = dev_ioctl
};

int dev_open(struct inode *minode, struct file *mfile){
    if(usage_count != 0){
        return -EBUSY;
    }
    usage_count = 1;

    return 0;
}

int dev_release(struct inode *minode, struct file *mfile){
    usage_count = 0;
    return 0;
}

long dev_ioctl(struct file *mfile, unsigned int ioctl_num, unsigned long ioctl_param){
    unsigned int datafromuser;
    switch(ioctl_num){
        case 0:
            if(copy_from_user(&datafromuser, (unsigned int*)ioctl_param, sizeof(datafromuser))){
                return -EFAULT;
            }
            data.fndpos = (datafromuser >> 24) & 0b11111111;
            data.fndval = (datafromuser >> 16) & 0b11111111;
            data.time_interval = (datafromuser >> 8) & 0b11111111;
            data.time_number = datafromuser & 0b11111111;
            data.cnt = 0;
            printk("%d %d %d %d %d\n", data.fndpos, data.fndval, data.time_interval, data.time_number, data.cnt);
            timer_function((unsigned long)&data);

            break;
        default:
            break;
    }
    return 0;
}

void init_device(void){
    int i;
    outw(0, (unsigned int)fnd_addr);
    outw(0, (unsigned int)led_addr);
    for(i = 0; i < 10; i++) outw(0, (unsigned int)dot_addr + i*2);
    for(i = 0; i < 32; i++) outw(0, (unsigned int)lcd_addr + i);
}

void timer_function(unsigned long data_ptr){
    int i;
    struct data_struct *data_p = (struct data_struct *)data_ptr;
    
    unsigned short int fndvalue;
    unsigned short ledvalue = 0;

    int dot_setnum = 0;
    int dot_strsize = 0;
    unsigned short int dotvalue;

    unsigned char lcd_num[16];
    unsigned char lcd_name[16];
    unsigned short int lcd_value;

    if(data_p->time_number == 0){
        init_device();
        return;
    }
    /* fnd */
    fndvalue = (data_p->fndval) << (4 * (4 - data_p->fndpos));
    outw(fndvalue, (unsigned int)fnd_addr);
    
    /* led */
    switch(data_p->fndval){
        case 1:
            ledvalue |= 0b10000000; break;
        case 2:
            ledvalue |= 0b01000000; break;
        case 3:
            ledvalue |= 0b00100000; break;
        case 4:
            ledvalue |= 0b00010000; break;
        case 5:
            ledvalue |= 0b00001000; break;
        case 6:
            ledvalue |= 0b00000100; break;
        case 7:
            ledvalue |= 0b00000010; break;
        case 8:
            ledvalue |= 0b00000001; break;
        default :
            printk("FND VALUE ERROR\n");
            break;
    }
    outw(ledvalue, (unsigned int)led_addr);

    /* dot */
    dot_setnum = data_p->fndval;
    if(dot_setnum < 0 || dot_setnum > 9){
        printk("Invalid Number In Dot operations, Not in (0~9)\n");
        return;
    }
    dot_strsize = sizeof(fpga_number[dot_setnum]);
    for(i = 0; i < dot_strsize; i++){
        dotvalue = fpga_number[dot_setnum][i] & 0x7F;
        outw(dotvalue, (unsigned int)dot_addr + i*2);
    }

    /* text lcd */
    memset(lcd_num, 0, sizeof(lcd_num));
    memcpy(lcd_num + lcd_num_pos, mynum, sizeof(mynum));
    for(i = 0; i < 16; i+=2){
        lcd_value = ((lcd_num[i] & 0xFF) << 8) | (lcd_num[i+1] & 0xFF);
        outw(lcd_value, (unsigned int)lcd_addr + i);
    }
    memset(lcd_name, 0, sizeof(lcd_name));
    memcpy(lcd_name + lcd_name_pos, myname, sizeof(myname));
    for(i = 0; i < 16; i+=2){
        lcd_value = ((lcd_name[i] & 0xFF) << 8) | (lcd_name[i+1] & 0xFF);
        outw(lcd_value, (unsigned int)lcd_addr + 16 + i);
    }
    lcd_num_pos += lcd_num_dir;
    lcd_name_pos += lcd_name_dir;
    if(lcd_num_pos > 8){
        lcd_num_pos = 7;
        lcd_num_dir *= -1;
    }
    if(lcd_num_pos < 0){
        lcd_num_pos = 1;
        lcd_num_dir *= -1;
    }
    if(lcd_name_pos > 4){
        lcd_name_pos = 3;
        lcd_name_dir *= -1;
    }
    if(lcd_name_pos < 0){
        lcd_name_pos = 1;
        lcd_name_dir *= -1;
    }

    /*
     * 3 4 5 6 7 8 1 2 3
     * 1 2 3 4 5 6 7 8 9
     * */
    printk("fnd count : %d\n", data_p->time_number);
    data_p->time_number -= 1; 
    data_p->cnt += 1;
    data_p->fndval += 1;
    if(data_p->cnt >= 8){
        data_p->cnt = 0;
        data_p->fndpos += 1;
        if(data_p->fndpos > 4){
            data_p->fndpos = 1;
        }
    }
    if(data_p->fndval > 8){
        data_p->fndval = 1;
    }

    
    timer.expires = get_jiffies_64() + (data_p->time_interval * HZ / 10);
    timer.data = (unsigned long)data_p;
    timer.function = timer_function;
    add_timer(&timer);
}

int __init m_init(void){
    /* REGISTER DEVICE DRIVER */
    int reg_flag = register_chrdev(MAJOR_NUMBER, DEVICE_DRIVER, &dev_fops);
    if(reg_flag < 0){
        printk("REGISTER DEVICE DRIVER ERROR, CHECK THE MAJOR_NUMBER.\n");
        return reg_flag;
    }
    printk("SUCCESS REGISTERING THE DEVICE DRIVER, DEVICE DRIVER NAME IS : %s, MAJOR NUMBER IS : %d\n", DEVICE_DRIVER, MAJOR_NUMBER);

    fnd_addr = ioremap(FND_ADDRESS, 0x04);
    led_addr = ioremap(LED_ADDRESS, 0x01);
    dot_addr = ioremap(DOT_ADDRESS, 0x10);
    lcd_addr = ioremap(LCD_ADDRESS, 0x32);
    init_timer(&timer);
    return 0;
}
void __exit m_exit(void){
    iounmap(fnd_addr);
    iounmap(led_addr);
    iounmap(lcd_addr);
    iounmap(dot_addr);
    del_timer_sync(&timer);
    unregister_chrdev(MAJOR_NUMBER, DEVICE_DRIVER);
}

module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");
