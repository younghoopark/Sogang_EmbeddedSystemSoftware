#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
/*
	FUNCTION NAME: main
	PARAMETER : void
	TASK : OPEN DEVICE FILE AND CALL WRITE FUNCTION
*/
int main(void){
    int fd;
    int retn;
    char buf = 0;
    fd = open("/dev/stopwatch", O_RDWR);
    if(fd < 0){
        perror("/dev/stopwatch open error");
        exit(-1);
    }
    else{
        printf("</dev/stopwatch has been detected > \n");
    }
    retn = write(fd, &buf, sizeof(buf));
    close(fd);

    return 0;
}
