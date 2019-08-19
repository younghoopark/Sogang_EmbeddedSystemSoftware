#include <linux/kernel.h>
#include <linux/uaccess.h>

asmlinkage unsigned int sys_retonevar(unsigned int t_interval, unsigned int t_number, unsigned int startoption)
{
    unsigned int retval = 0;
    unsigned char fndpos, fndval, time_interval, time_number;
    /* startoption을 통해 fndpos, fndval값 구하기 */
    /* startoption 총 2Byte(16Bit) */
    /*
     *  startoption[0]   startoption[1]
     *  ----    ----     ----    ----
     *   0       0        4       0      ==>  3번째 자리에서 4번째 문양부터 출력을 시작한다.
     *
     * */

    unsigned char check[4];
    //unsigned int mask[4] = {0x0F000000, 0x000F0000, 0x00000F00, 0x0000000F};
    //unsigned int temp = startoption;
    //check[0] = ((startoption & mask[0]) >> 24)
    //check[1] = ((startoption & mask[1]) >> 16);
    //check[2] = ((startoption & mask[2]) >> 8);
    //check[3] = (startoption & mask[3]);
    check[0] = startoption / 1000;
    check[1] = (startoption % 1000) / 100;
    check[2] = (startoption % 100) / 10;
    check[3] = (startoption % 10) / 1;
    //printk("%d %d %d %d\n", check[0], check[1], check[2], check[3]);
    if(check[0] > 8 || check[1] > 8 || check[2] > 8 || check[3] > 8){
        printk("startoption error!!\n");
        return 0;
    }
    if(check[0]){
        fndpos = 1, fndval = check[0];
    }
    else if(check[1]){
        fndpos = 2, fndval = check[1];
    }
    else if(check[2]){
        fndpos = 3, fndval = check[2];
    }
    else if(check[3]){
        fndpos = 4, fndval = check[3];
    }
    else{
        printk("startoption boundary error!! MUST IN [0001 - 8000].\n");
        return 0;
    }

    time_interval = t_interval;
    time_number = t_number;

    retval = (fndpos << 24) + (fndval << 16) + (time_interval << 8) + time_number;
    return retval;      /* fndpos(8bit), fndval(8bit), time_interval(8bit), time_number(8bit) */
}
