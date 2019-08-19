#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define BUFF_SIZE 64
#define MAX_BUTTON 9
#define KEY_RELEASE 0
#define KEY_PRESS 1

#define MODE_CLOCK 1
#define MODE_COUNTER 2
#define MODE_TEXTEDITOR 3
#define MODE_DRAWBOARD 4

#define MAX_LCD 32

#define KEY_DEVICE "/dev/input/event0"
#define SWITCH_DEVICE "/dev/fpga_push_switch"
#define FND_DEVICE "/dev/fpga_fnd"
#define LCD_DEVICE "/dev/fpga_text_lcd"
#define LED_DEVICE "/dev/mem"
#define DOT_DEVICE "/dev/fpga_dot"
#define MOT_DEVICE "/dev/fpga_step_motor"

#define FPGA_BASE_ADDRESS 0x08000000
#define LED_ADDR 0x16
#define ONESEC 750

typedef struct MSG_{
    long mtype;
    int mode;
    int keyvalue, keycode; //pushed release
    int key[3];   //BACK, VOL-, VOL+
    int endflag;  //back
    unsigned char switches[MAX_BUTTON]; //switch
    char fnd[4]; //fnd
    char lcd[MAX_LCD];
    int dotmode;
    char textdot[2][10];
    char drawdot[10];
}MSG;

int readkeyflag = 0;
int readswitchflag = 0;
static unsigned char quit = 0;
static int mode;
static int end_flag = 0;

int modechange = 0;
int fd_fnd, fd_lcd, fd_dot, fd_led;
/* mode3 */
char* textboard[MAX_BUTTON] = {".QZ","ABC","DEF","GHI","JKL","MNO","PRS","TUV","WXY"};
char dot[2][10] = {{0x1c,0x36,0x63,0x63,0x63,0x7f,0x7f,0x63,0x63,0x63},
                    {0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}};
