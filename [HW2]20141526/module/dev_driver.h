#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>

#include <asm/ioctl.h>

#define MAJOR_NUMBER 242
#define DEVICE_DRIVER "dev_driver"

#define FND_ADDRESS 0x08000004
#define LED_ADDRESS 0x08000016
#define LCD_ADDRESS 0x08000090
#define DOT_ADDRESS 0x08000210

#define LINE_BUFF 16

struct data_struct{
    unsigned char fndpos;
    unsigned char fndval;
    unsigned char time_interval;
    unsigned char time_number;
    unsigned char cnt;
};

#ifndef __FPGA_NUMBER__
#define __FPGA_NUMBER__

unsigned char fpga_number[10][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
	{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
};

unsigned char fpga_set_full[10] = {
	// memset(array,0x7e,sizeof(array));
	0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f
};

unsigned char fpga_set_blank[10] = {
	// memset(array,0x00,sizeof(array));
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
#endif

static unsigned char *fnd_addr;
static unsigned char *led_addr;
static unsigned char *lcd_addr;
static unsigned char *dot_addr;

static const char mynum[8] = {"20141526"};
static const char myname[12] = {"ParkYoungHoo"};

//static const char mynum[8] = {'2','0','1','4','1','5','2','6'};
//static const char myname[12] = {'P','a','r','k','Y','o','u','n','g','H','o','o'};
struct timer_list timer;
struct data_struct data;
int usage_count = 0;
int lcd_num_pos = 0;
int lcd_name_pos = 0;
int lcd_num_dir = 1;
int lcd_name_dir = 1;

/* FUNCTIONS */
int dev_open(struct inode*, struct file*);
int dev_release(struct inode*, struct file*);
long dev_ioctl(struct file*, unsigned int, unsigned long);
//ssize_t dev_write(struct file*, const char*, size_t, loff_t*);
void init_device(void);
static void timer_function(unsigned long);
/* FUNCTIONS */


