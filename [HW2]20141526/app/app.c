#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <asm/ioctl.h>
#include <string.h>
#include <stdlib.h>

#define DEVICE_NAME "/dev/dev_driver"
int main(int argc, char **argv)
{
    unsigned int data = 0;
    unsigned int interval, number, startoption;
    if(argc != 4){
        printf("Invalid argument, put parameters (interval, number, startoption).\n");
        return -1;
    }
    interval = (unsigned int)atoi(argv[1]);
    number = (unsigned int)atoi(argv[2]);
    startoption = (unsigned int)atoi(argv[3]);
    if(interval < 1 || interval > 100){
        printf("Invalid argument, The (first parameter)time interval's boundary is [1 - 100].\n");
        return -1;
    }
    if(number < 1 || number > 100){
        printf("Invalid argument, The (second parameter)number boundary is [1 - 100].\n");
        return -1;
    }
    if(startoption < 0 || startoption > 8000){
        printf("Invalid argument, The (third parameter)startoption boundary is [0000 - 8000]\n");
        return -1;
    }
    
    /* GET ONE VARIABLE FROM THE SYSTEM CALL */
    data = syscall(376, interval, number, startoption);    
    if(data == 0){
        return -1;
    }

    int fd = open(DEVICE_NAME, O_WRONLY);
    if(fd < 0){
        printf("%s DEVICE FILE OPEN ERROR.\n", DEVICE_NAME);
        return -1;
    }
    //ioctl(fd, _IOW(242, 0, unsigned int), &data);
    ioctl(fd, 0, &data);
    close(fd);

    /*int i;
    int mask;
    for(i = 31; i >= 0; i--){
        mask = 1 << i;
        printf("%d", data & mask ? 1 : 0);
        if(i % 8 == 0){
            printf(" ");
        }
    }
    printf("\n");
    */

    return 0;
}
