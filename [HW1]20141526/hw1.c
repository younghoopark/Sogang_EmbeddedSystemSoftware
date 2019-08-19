#include "hw1.h"
/* FUNCTION NAME : initialize_msg
 * PARAMETER : MSG *buf
 * THIS FUNCTION INITIALIZE THE MSG VARIABLE
 * */
void initialize_msg(MSG *buf){
    memset((*buf).key, 0, sizeof((*buf).key));
    memset((*buf).switches, 0, sizeof((*buf).switches));
    memset((*buf).fnd, 0, sizeof((*buf).fnd));

    memset((*buf).lcd, 0, sizeof((*buf).lcd));
    memset((*buf).textdot, 0, sizeof((*buf).textdot));
    memset((*buf).drawdot, 0, sizeof((*buf).drawdot));
}

/* FUNCTION NAME : initialize
 * PARAMETER : MSG *buf
 * THIS FUNCTION INITIALIZE THE MSG VARIABLE AND THE TARGET DEVICE
 * */
void initialize(MSG *buf){
    initialize_msg(buf);
    char fnd[4];
    memset(fnd, 0, sizeof(fnd));
    char lcd[MAX_LCD];
    memset(lcd, 0, sizeof(lcd));
    char dot[10];
    memset(dot, 0, sizeof(dot));
    /* WRITE ON TARGET DEVICE THE INITIALIZED VALUE */
    write(fd_dot, dot, 10);
    write(fd_fnd, fnd, 4);
    write(fd_lcd, lcd, 32);
}

/* FUNCTION NAME : change_mode
 * PARAMETER : MSG *buf
 * THIS FUNCTION CHANGE THE MODE ACCORDING TO THE PUSHED BUTTON (VOL- || VOL+)
 * */
void change_mode(MSG *buf){
    /* VOL+ */
	if((*buf).key[2]){
		if(mode == MODE_DRAWBOARD){
			mode = MODE_CLOCK;
		}
		else{
			mode += 1;
		}
	}
    /* VOL- */
	else if((*buf).key[1]){
		if(mode == MODE_CLOCK){
			mode = MODE_DRAWBOARD;
		}
		else{
			mode -= 1;
		}
	}
    /* AFTER CHANGE THE MODE, INITIALIZE THE BOARD CONDITION */
    initialize(buf);
	
	printf("MODE : %d\n", mode);
}
/* FUNCTION NAME : func_clock
 * PARAMETERS : MSG *buf, unsigned char *led_addr
 * THIS FUNCTION IS A FUNCTION FOR MODE1
 * SAVE INFORMATION AT THE PARAMETER MSG *buf AND UPDATE THE LED ON DEVICE
 * USING MMAP
 * */
void func_clock(MSG *buf, unsigned char *led_addr){
    (*buf).mode = mode;
    static int editmode = 0, init = 0, flag = 0;
    static clock_t s, t;
    //printf("clock_t s : %d, clock_t t : % d, clock() : %d\n", s, t, clock());
    time_t rawtime = time(NULL);
    static struct tm timeinfo;

    /* IF INIT = 0, SET THE timeinfo VARIABLE AS THE LOCALTIME AND UPDATE init = 1 */
    if(init == 0) {
        timeinfo = *localtime(&rawtime);
        init = 1;
    }
    
    /* SET THE led_addr */
    *led_addr |= 128;
    time(&rawtime);
    
    /* WHEN PUSHED VOL- OR VOL+ */
    if((*buf).key[1] || (*buf).key[2]){
        timeinfo = *localtime(&rawtime);
        editmode = 0;
    }
    /* WHEN PUSHED THE SWITCH1, SET THE TIME EDIT MODE */
    if((*buf).switches[0]){
        flag = 0;
        editmode ^= 1;
        printf("clock edit mode\n");
    }
    /* TIME EDIT MODE */
    if(editmode){
        /* LED UPDATES EVERY SECOND */
        if(clock() - s > ONESEC){
            if(flag == 0){
                *led_addr ^= 16;
                flag = 1;
            }
            else{
                *led_addr ^= 48;
            }
            s = clock();
        }
        /* WHEN PUSHED THE SWITCH2, RESET THE TIME AS A LOCALTIME */
        if((*buf).switches[1]){
            timeinfo = *localtime(&rawtime);
        }
        /* WHEN PUSHED THE SWITCH3 */
        if((*buf).switches[2]) timeinfo.tm_hour += 1;
        /* WHEN PUSHED THE SWITCH4 */
        if((*buf).switches[3]) timeinfo.tm_min += 1;
    }
    /* UPDATE THE TIME */
    else if(clock()-t > ONESEC){
        *led_addr &= 128;
        timeinfo.tm_sec += 1;
        t = clock();
    }
    timeinfo.tm_hour += (timeinfo.tm_min)/60; timeinfo.tm_hour %= 24;
    timeinfo.tm_min += (timeinfo.tm_sec)/60; timeinfo.tm_min %= 60;
    timeinfo.tm_sec %= 60;
    /* SAVE THE UPDATED INFORMATION IN THE *buf */
    (*buf).fnd[0] = timeinfo.tm_hour / 10;
    (*buf).fnd[1] = timeinfo.tm_hour % 10;
    (*buf).fnd[2] = timeinfo.tm_min / 10;
    (*buf).fnd[3] = timeinfo.tm_min % 10;
    /* WHEN PUSHED BACK */
    if((*buf).endflag == 1){
        *led_addr = 0;      /* INITIALIZE LED ON THE DEVICE USING MMAP */
        initialize(buf);    /* INITIALIZE THE TARGET DEVICE BOARD */
    }
}
/* FUNCTION NAME : func_counter
 * PARAMETERS : MSG *buf, unsigned char *led_addr
 * THIS FUNCTION IS A FUNCTION FOR MODE2
 * SAVE INFORMATION AT THE PARAMETER MSG *buf AND UPDATE THE LED ON THE DEVICE
 * USING MMAP
 * */
void func_counter(MSG *buf, unsigned char *led_addr){
    static int counter = 0;
    /* ARRANGEMENT FOR CALCULATION ACCORDING TO THE BASE METHOD */
    char basearr[4] = {10, 8, 4, 2};
    static int baseidx = 0;
    int base;
    
    /* ARRANGEMENT FOR LED ACCORDING TO THE BASE METHOD */
    char led[4] = {64, 32, 16, 128};
    int i;

    if(counter == 0){
        *led_addr = led[0];
    }
    *led_addr = led[baseidx];
    if((*buf).key[1] || (*buf).key[2]){     //VOL- || VOL+
        counter = 0; baseidx= 0;
        *led_addr = led[0];
    }
    /* WHEN PUSHED THE SWITCH1 */
    if((*buf).switches[0]){
        baseidx = (baseidx+1)%4;
        printf("BASE : %d\n", basearr[baseidx]);
    }
    /* WHEN PUSHED THE SWITCH2 */
    if((*buf).switches[1]){
        counter += (basearr[baseidx] * basearr[baseidx]);
    }
    /* WHEN PUSHED THE SWITCH3 */
    if((*buf).switches[2]){
        counter += basearr[baseidx];
    }
    /* WHEN PUSHED THE SWITCH4 */
    if((*buf).switches[3]){
        counter += 1;
    }
    
    /* SAVE THE INFORMATION ABOUT THE FND IN THE PARAMETER MSG *buf */
    int tmpcounter;
    tmpcounter = counter;
    base = (int)basearr[baseidx];
    for(i = 3; i >= 0; --i){
        (*buf).fnd[i] = tmpcounter % base;
        tmpcounter /= base;
    }
    (*buf).fnd[0] = 0;
    /* WHEN PUSHED BACK */
    if((*buf).endflag == 1){
        *led_addr = 0;      /* INITIALIZE LED ON THE DEVICE USING MMAP */
        initialize(buf);    /* INITIALIZE THE TARGET DEVICE BOARD */
    }

}
/* FUNCTION NAME : func_texteditor
 * PARAMETERS : MSG *buf
 * THIS FUNCTION IS A FUNCTION FOR MODE3
 * SAVE INFORMATION AT THE PARAMETER MSG *buf
 * */
void func_texteditor(MSG *buf){
    char fnd[4];
    memset(fnd, 0, sizeof(fnd));
    char lcd[MAX_LCD];
    static int boardidx = 0, csr = -1, textmode = 0, counter = 0;
    int i;

    if(csr == -1){
        memset(lcd, 0, sizeof(lcd));
        write(fd_lcd, lcd, 32);
        csr = 0;
    }
    /* WHEN PUSHED VOL- OR VOL+, CHANGE THE MODE, SO INITIALIZE THE STATIC VARIABLES */
    if((*buf).key[1] || (*buf).key[2]){
        counter = 0; 
        textmode = 0;
        csr = -1;
        (*buf).dotmode = textmode;
        return;
    }
    /* WHEN PUSHED SWITCH2 AND SWITCH3 AT THE SAME TIME 
     * CLEAR THE TEXT LCD */
    if((*buf).switches[1] && (*buf).switches[2]){
        csr = -1;
        ++counter;
        return ;
    }
    /* WHEN PUSHED SWITCH5 AND SWITCH6 AT THE SAME TIME 
     * CHANGE THE TEXTMODE(ENGLISH OR NUMBER */
    if((*buf).switches[4] && (*buf).switches[5]){
        textmode ^= 1;
        ++counter;
        (*buf).dotmode = textmode;
        return;
    }
    /* WHEN PUSHED SWITCH8 AND SWITCH9 AT THE SAME TIME 
     * PUT SPACE ON LCD */
    if((*buf).switches[7] && (*buf).switches[8]){
        csr++; lcd[csr] = ' '; counter++;
    }
    /* TEXTMODE0 ENGLISH */
    else if(!textmode){
        if(lcd[csr] == 0){
            for(i = 0; i < MAX_BUTTON; i++){
                if((*buf).switches[i]) lcd[csr] = textboard[i][0];
            }
        }
        for(i = 0; i < 9; i++){
            if((*buf).switches[i]){    
                if(i != 0 && textboard[i][0] <= lcd[csr] && lcd[csr] <= textboard[i][2]){
                    boardidx = (boardidx+1) % 3;
                    lcd[csr] = textboard[i][boardidx];
                }
                else if((i == 0) && (lcd[csr] == 'Q' || lcd[csr] == 'Z' || lcd[csr] == '.')){
                    boardidx = (boardidx+1) % 3;
                    lcd[csr] = textboard[0][boardidx];
                }
                else{
                    ++csr;
                    boardidx = 0;
                    lcd[csr] = textboard[i][0];
                }
                /* UPDATE COUNTER */
                ++counter;
            }
        }
    }
    /* TEXTMODE1 NUMBER */
    else{
        for(i = 0; i < 9; i++){
            if((*buf).switches[i]){
                if(lcd[csr] == 0) lcd[csr] = '1'+i;
                else lcd[++csr] = '1'+i;
                ++counter;
            }
        }
    }
    /* WHEN BEYOND THE MAXIMUM POWER RANGE */
    if(csr >= 8){
        --csr;
        for(i = 0; i < 8; i++){
            lcd[i] = lcd[i+1];
        }
    }
    int tmpcounter;
    tmpcounter = counter;
    for(i = 3; i >= 0; --i){
        fnd[i] = tmpcounter % 10; tmpcounter /= 10;
    }
    /* UPDATE MSG *buf ACCORDING TO THE INFORMATION FROM THIS FUNCTION */
    (*buf).dotmode = textmode;
    memcpy((*buf).lcd, lcd, sizeof(lcd));
    memcpy((*buf).fnd, fnd, sizeof(fnd));
    memcpy((*buf).textdot, dot, sizeof(dot));

}

/* FUNCTION NAME : func_drawboard
 * PARAMETERS : MSG *buf
 * THIS FUNCTION IS A FUNCTION FOR MODE4
 * SAVE INFORMATION AT THE PARAMETER MSG *buf
 * */
void func_drawboard(MSG *buf){
    static char dot[10] = {0};
    char fnd[4];
    memset(fnd, 0, sizeof(fnd));
    static clock_t t = 0;
    static int r = 0, c = 64, drawmode = 0, counter = 0, flag = 0;
    int i;
    /* WHEN PUSHED VOL- || VOL+, IT MEANS CHANGE MODE, SO
     * INITIALIZE THE STATIC VARIABLES */
    if((*buf).key[1] || (*buf).key[2]){   //VOL- || VOL+
        memset(dot, 0, sizeof(dot));
        r = 0; c = 64; counter = 0; drawmode = 0;
    }
    /* WHEN PUSHED SWITCH1 */
    if((*buf).switches[0]){
        memset(dot, 0, sizeof(dot));
        r = 0; c = 64; counter += 1;
    }
    /* WHEN PUSHED SWITCH2 */
    if((*buf).switches[1]){
        if(r > 0){
            if(flag % 2){
                flag = 0;
                dot[r] ^= c;
            }
            --r;
        }
        ++counter;
    }
    /* WHEN PUSHED SWITCH3 */
    if((*buf).switches[2]){
        if(flag % 2){
            flag = 0;
            dot[r] ^= c;
        }
        drawmode ^= 1;
        ++counter;
    }
    /* WHEN PUSHED SWITCH4 */
    if((*buf).switches[3]){
        if(c < 64){
            if(flag % 2){
                flag = 0;
                dot[r] ^= c;
            }
            c <<= 1;
        }
        ++counter;
    }
    /* WHEN PUSHED SWITCH5 */
    if((*buf).switches[4]){
        dot[r] ^= c; 
        ++counter;
    }
    /* WHEN PUSHED SWITCH6 */
    if((*buf).switches[5]){
        if(c > 1){
            if(flag % 2){
                flag = 0;
                dot[r] ^= c;
            }
            c >>= 1;
        }
        ++counter;
    }
    /* WHEN PUSHED SWITCH7 */
    if((*buf).switches[6]){
        memset(dot, 0, sizeof(dot));
        ++counter;
    }
    /* WHEN PUSHED SWITCH8 */
    if((*buf).switches[7]){
        if(r < 9){
            if(flag % 2){
                flag = 0;
                dot[r] ^= c;
            }
            ++r;
        }
        ++counter;
    }
    /* WHEN PUSHED SWITCH9 */
    if((*buf).switches[8]){
        for(i = 0; i < 10; i++){
            dot[i] = dot[i] ^ 127;
        }
        ++counter;
    }
    
    /* (WHEN PUSHED SWITCH3), UPDATE LED ON THE DEVICE EVERY SECOND */
    if(!drawmode){          //깜빡이는 부분
        if((clock() - t > ONESEC)){
            ++flag;
            dot[r] ^= c;
            t = clock();
        }
    }
    
    /* UPDATE ARRAY fnd AND
     * SAVE THIS INFORMATION IN THE PARAMETER MSG *buf, TO SEND OUTPUT PROCESS */
    int tmpcounter;
    tmpcounter = counter;
    for(i = 3; i >= 0; --i){
        fnd[i] = tmpcounter % 10; tmpcounter /= 10;
    }
    memcpy((*buf).fnd, fnd, sizeof(fnd));
    memcpy((*buf).drawdot, dot, sizeof(dot));
}

/* THREAD[2] FUNCTION
 * THIS THREAD READ 4KEYS(BACK,PROG,VOL+,VOL-) FROM THE TARGET BOARD, AND SEND TO MAIN PROCESS USING THE MSG QUEUE
 * */
void *read_four_keys(){
	static int retval = 999;
    MSG buf;
    struct input_event ev[BUFF_SIZE];
	int fd, rd, value, size = sizeof(struct input_event);
	char name[256] = "Unknown";
	char* device = "/dev/input/event0";
	if((fd = open(device, O_RDONLY)) == -1){
		printf("%s is not a valid device.n", device);
	}

    /* CREATE MSG QUEUE BETWEEN INPUT - MAIN */
    key_t key = 3000;           //input -> main queue
    int que_id = msgget(key, IPC_CREAT | 0666);
	if(que_id < 0){
        perror("msgget error : ");
        close(fd);
        pthread_exit((void*)&retval);
    }
	
    /* THE FLOG VARIABLE FOR CHECK BREAK CONDITION */
	int quit_test = false;
    int send_data_size;
	while(!quit_test){
		if ((rd = read (fd, ev, size * BUFF_SIZE)) < size){
			quit_test = true;
		}
        /* value SAVE THE INFORMATION ABOUTOUT KEY_RELEASE OR KEY_PRESS */
		value = ev[0].value;
        int pushed_button = ev[0].code;
        
        if(value == KEY_PRESS && readkeyflag == 0){
            initialize_msg(&buf);
            buf.keycode = pushed_button;
            buf.keyvalue = value;
            /* MTYPE = 1 MEANS BETWEEN INPUT - MAIN QUEUE */
            buf.mtype = 1;
            send_data_size = sizeof(MSG) - sizeof(long);
            /* WHEN PUSHED VOL- */
            if(pushed_button == 114){       //VOL-
                buf.key[1] = 1;
            }
            /* WHEN PUSHED VOL+ */
            if(pushed_button == 115){       //VOL+
                buf.key[2] = 1;
            }
            /* WHEN PUSHED BACK */
            if(pushed_button == 158){           //BACK키 눌렸을 때
                buf.endflag = 1;
                buf.key[0] = 1;
                quit_test = true;
            }
            /* PUSH THE MSG IN THE MSG QUEUE BETWEEN INPUT - MAIN */
            if(msgsnd(que_id, &buf, send_data_size, IPC_NOWAIT) == -1){
                perror("msgsnd error : ");
                break;
            }
            readkeyflag = 1;
        }
        if(value == KEY_RELEASE){
            readkeyflag = 0;
        }
    }
    /* THE SIGNAL TO THE OTHER THREAD(read_switches) */
    end_flag = 1;
    close(fd);
    /* END THIS THREAD */
    pthread_exit((void*)&retval);
}

/* THREAD[1] FUNCTION
 * THIS THREAD READ SWITCHES(0~9) FROM THE TARGET BOARD, AND SEND TO MAIN PROCESS USING THE MSG QUEUE
 * */
void *read_switches(){
	static int retval = 999;
    int i;
	int dev;
	int buff_size;
	unsigned char push_sw_buff[MAX_BUTTON];
    MSG buf;

	dev = open("/dev/fpga_push_switch", O_RDWR);
	if(dev < 0){
		printf("Device Open Error\n");
		close(dev);
		return;
	}
    
    /* CREATE MSG QUEUE */
    key_t key = 3000;       //input -> main queue
    int que_id = msgget(key, IPC_CREAT | 0666);
	if(que_id < 0){
        perror("msgget error : ");
        close(dev);
        pthread_exit((void*)&retval);
    }
    buf.mtype = 1;   
	buff_size = sizeof(push_sw_buff);
    int send_data_size = sizeof(MSG) - sizeof(long);
	printf("Press <ctrl+c> to quit. \n");
    while(!quit){
        initialize_msg(&buf);
        usleep(100000);
        read(dev, push_sw_buff, buff_size);
        for(i = 0; i < MAX_BUTTON; i++){
            if(push_sw_buff[i] == 1){
                memcpy(buf.switches, push_sw_buff, sizeof(push_sw_buff));
            }
        }
        msgsnd(que_id, &buf, send_data_size, 0);
        
        if(end_flag == 1){  //BACK눌렀을 때
            break;
        }
    }
    
    close(dev);
    /* END THIS THREAD */
    pthread_exit((void*)&retval);
}

/* INPUT PROCESS */
/* INPUT PROCESS HAS TWO THREADS 
 * THREAD[0] READ THE FOUR_KEYS FROM THE TARGET BOARD, AND SEND IT TO MAIN PROCESS USING THE MSG QUEUE
 * THREAD[1] READ THE SWITCHES(0~9) FROM THE TARGET BOARD, AND SEND IT TO MAIN PROCESS USING THE MSG QUEUE
 *
 * THIS PROCESS SEND THE DATAS READ FROM DEVICE TO THE MAIN PROCESS
 * */
void input_proc(){
	/* Thread Variables */
	pthread_t p_thread[2];
	int thr_id; /* thread generation error check */
    int status;
    thr_id = pthread_create(&p_thread[0], NULL, read_four_keys, NULL);
    if(thr_id < 0){
        perror("thread[0] create error :");
        exit(0);
    }

    thr_id = pthread_create(&p_thread[1], NULL, read_switches, NULL);
    if(thr_id < 0){
        perror("thread[1] create error :");
        exit(0);
    }
    pthread_join(p_thread[0], 0);
    pthread_join(p_thread[1], 0);
}

/* OUTPUT PROCESS
 * WRITE ON THE TARGET BOARD ACCORDING TO EACH MODE
 * WRITE THE DATAS IN THE MSG *buf RECEIVED FROM MAIN PROCESS
/* */
void output_proc(){
    MSG buf;
    /* GET THE MSG QUEUE ID, THIS QUEUE IS BETWEEN MAIN - OUTPUT */
    key_t key = 4000;
    int que_id = msgget(key, IPC_CREAT | 0666);
    if(que_id < 0){
        perror("Keys msgget error: ");
        exit(0);
    }
    /* SET THE MSG SIZE SENT BY MAIN PROCESS */
    int get_data_size = sizeof(MSG) - sizeof(long);
    while(1){
        /* RECEIVE MSG FROM THE MAIN PROCESS */
        msgrcv(que_id, &buf, get_data_size, 1, 0);
        /* IF PUSHED VOL- || VOL +, INITIALIZE THE BUF */
        if(buf.key[1] || buf.key[2]){
            initialize(&buf);
        }
        /* BREAK OUTPUT PROCESS LOOP */
        if(buf.endflag == 1){
            break;
        }
        
        /* PRINT ON DEVICE ABOUT THE MSG, AND THE MODE */
        switch(buf.mode){
            case MODE_CLOCK:
                write(fd_fnd, buf.fnd, 4);
                break;
            case MODE_COUNTER:
                write(fd_fnd, buf.fnd, 4);
                break;
            case MODE_TEXTEDITOR:
                write(fd_dot, buf.textdot[buf.dotmode], 10);
                write(fd_fnd, buf.fnd, 4);
                write(fd_lcd, buf.lcd, 8);
                break;
            case MODE_DRAWBOARD:
                write(fd_dot, buf.drawdot, 10);
                write(fd_fnd, buf.fnd, 4);
                break;
        }
    }
    /* AFTER BREAK OUTPUT PROCESS LOOP, INITIALIZE THE BUF, AND PRINT THE INITIALIZED DATA ON THE TARGET BOARD */
    initialize(&buf);
    /* DELETE THE MSG QUEUE BETWEEN MAIN - OUTPUT */
    msgctl(que_id, IPC_RMID, NULL);
}

/* MAIN PROCESS */
/* THIS PROCESS GET THE MSG FROM THE INPUT PROCESS WHICH READ FROM THE DEVICE, AND
 * AFTER PERFORMING THE CACULATION ACCORDING TO EACH MODE, SEND THE RESULT TO THE OUTPUT PROCESS BY USING
 * MSG QUEUE
 * */
void main_proc(){
    /* THE DEFAULT MODE IS MODE_CLOCK(1) */
    mode = MODE_CLOCK;
    MSG buf;
    /* GET THE MSG QUEUE ID, THIS QUEUE IS BETWEEN INPUT - MAIN */
    key_t key = 3000;
    int que_id = msgget(key, IPC_CREAT | 0666);
    if(que_id < 0){
        perror("Keys msgget error: ");
        exit(0);
    }
    /* GET THE MSG QUEUE ID. THIS QUEUE IS BETWEEN MAIN - OUTPUT */
    key_t key2 = 4000;
    int que_id2 = msgget(key2, IPC_CREAT | 0666);
    if(que_id2 < 0){
        perror("Keys msgget error: ");
        exit(0);
    }
    /* SET THE SIZE OF MSG SENT FROM INPUT PROCESS */
    int get_data_size = sizeof(MSG) - sizeof(long);
    /* SET THE SIZE OF MSG TO OUTPUT PROCESS */
    int send_data_size = sizeof(MSG) - sizeof(long);
    unsigned long *fpga_addr = 0;
    unsigned char *led_addr = 0;
    unsigned char led;
    
    /* INITIALIZE THE LED ADDRESS VARIABLE */
    led = 128;
    fpga_addr = (unsigned long*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd_led, FPGA_BASE_ADDRESS);
    led_addr = (unsigned char*)((void*)fpga_addr+LED_ADDR);
    *led_addr = led;
    while(1){
        /* RECEIVE THE MSG FROM THE INPUT PROCESS */
        if( msgrcv(que_id, &buf, get_data_size, 1, 0) == -1){
            perror("msgrcv error : ");
            exit(0);
        }
        /* CALL THE FUNCTIONS ABOUT THE MODE */
        switch(mode){
            case MODE_CLOCK:
                func_clock(&buf, led_addr);
                break;
            case MODE_COUNTER:
                func_counter(&buf, led_addr);
                break;
            case MODE_TEXTEDITOR:
                func_texteditor(&buf);
                break;
            case MODE_DRAWBOARD:
                func_drawboard(&buf);
                break;
        }
        /* IF PUSHED THE PROG */
        if(buf.keycode == 116){
            printf("THIS BUTTON IS PROG\n");
        }
        /* IF PUSHED THE VOL+ OR VOL- */
        if(buf.keycode == 115 || buf.keycode == 114){
            change_mode(&buf);
            *led_addr = led;
            if(mode == 3 || mode == 4){
                *led_addr = 0;
            }
        }
        buf.mode = mode;
        
        /* SEND MSG TO THE QUEUE BETWEEN MAIN - OUTPUT */
        msgsnd(que_id2, &buf, send_data_size, IPC_NOWAIT);

        /* BREAK MAIN PROCESS LOOP */
        if(buf.endflag == 1){
            break;
        }
    }
    /* DELETE THE QUEUE BETWEEN INPUT - MAIN */
    msgctl(que_id, IPC_RMID, NULL);
}

int main()
{   
    /* DEVICE OPEN */
    if((fd_fnd = open(FND_DEVICE,O_RDWR)) < 0){
        printf("FND Device open error\n");
    }
    if((fd_lcd = open(LCD_DEVICE,O_RDWR)) < 0){
        printf("LCD Device open error\n");
    }
    if((fd_dot = open(DOT_DEVICE,O_RDWR)) < 0){
        printf("DOT Device open error\n");
    }
    if((fd_led = open(LED_DEVICE,O_RDWR)) < 0){
        printf("LED Device open error\n");
    }

    /* MAKE TWO CHILD PROCESSES, INPUT_PROCESS, OUTPUT_PROCESS */
    pid_t pid_in = 0, pid_out = 0;
	int status;
    mode = MODE_CLOCK;		//setting default mode(clock)
   
    pid_in = fork();
    if(pid_in < 0){
        printf("fork failure\n");
    }
    if(pid_in){
        pid_out = fork();
        if(pid_out < 0){
            printf("fork failure\n");
        }
    }
    
    if(!pid_in){            //input process
        pid_out = -1;
        input_proc();
    }
    if(!pid_out){           //output process
        output_proc();
    }

    /* MAIN PROCESS */
    if(pid_in && pid_out){
        main_proc();
        pid_in = wait(&status);
        pid_out = wait(&status);
    }

    /* ClOSE DEVICE */
    close(fd_fnd);
    close(fd_lcd);
    close(fd_dot);
    close(fd_led);
	return 0;
}
