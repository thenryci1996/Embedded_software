#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "io.h"
#include "altera_avalon_performance_counter.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_fifo_util.h"
#include "altera_avalon_fifo_regs.h"
#include "altera_avalon_mutex.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"



#define TRUE 1
#define SECTION_1 1
#define SECTION_2 2
#define SECTION_3 3
#define SECTION_4 4


#define LED_RED_0 0x00000001 // Engine
#define LED_RED_1 0x00000002 // Top Gear
#define LED_RED_2 0x00000004 // LED_RED_2
#define LED_RED_3 0x00000008 
#define LED_RED_12 0x00001000 // LED_RED_12
#define LED_RED_13 0x00002000 // LED_RED_13
#define LED_RED_14 0x00004000 // LED_RED_14
#define LED_RED_15 0x00008000 // LED_RED_15
#define LED_RED_16 0x00010000 // LED_RED_16
#define LED_RED_17 0x00020000


#define LED_GREEN_0 0x0001 
#define LED_GREEN_2 0x0002 
#define LED_GREEN_4 0x0010 
#define LED_GREEN_6 0x0040 
extern void delay (int millisec);

alt_mutex_dev *mutex_0;
alt_mutex_dev *mutex_1;
alt_mutex_dev *mutex_2;



int readFromFifo(int fifoId, int *tab);
int writeToShared(int cpu_id, int value, int offset);
void checkSwitch();
int pollKeys();
int readFromShared(int index, int *sharedAddress);
void loopFunction(int n);
static int b2sLUT[] = {0x40, //0
                 0x79, //1
                 0x24, //2
                 0x30, //3
                 0x19, //4
                 0x12, //5
                 0x02, //6
                 0x78, //7
                 0x00, //8
                 0x18, //9
                 0x3F, //-
};

/*
 * convert int to seven segment display format
 */
int int2seven(int inval){
    return b2sLUT[inval];
}

int sev_seg_decoder(int inval)
{
    int temp = 0, final = 0;
    temp = inval%10;
    final =final + int2seven(temp);
    inval = inval/10;
    temp = inval%10;
    final = final + (int2seven(temp)<<8);
    inval = inval/10;
    temp = inval%10;
    final = final + (int2seven(temp)<<16);
    inval = inval/10;
    temp = inval%10;
    final = final + (int2seven(temp)<<24);
    return ~final;
}


int main()
{
    
    int keysValue ;
    int hexTab[4];
    int res;
    int i = 0;
    
    /* init the performance counter , the fifos , the mutexes */
    mutex_0 = altera_avalon_mutex_open(MUTEX_0_NAME);
    mutex_1 = altera_avalon_mutex_open(MUTEX_1_NAME);
	mutex_2 = altera_avalon_mutex_open(MUTEX_2_NAME);
	
	
	res = altera_avalon_fifo_init(FIFO_0_IN_CSR_BASE, 0, 1, FIFO_0_IN_CSR_FIFO_DEPTH - 4);
	
    printf("Hello from cpu_0!\n");
	/* reset the performance counter */
	
    /* Start Measuring */
      
    //printf("writing to the fifo \n");
    
    /* write 5 to fifo */
    altera_avalon_mutex_lock(mutex_0, 1);
    /* after the mutex because we want to measure only the transfer time */
    PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
    PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);  
    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
    altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE, FIFO_0_IN_CSR_BASE, 1);
    altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE, FIFO_0_IN_CSR_BASE, 2);
    altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE, FIFO_0_IN_CSR_BASE, 3);
    altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE, FIFO_0_IN_CSR_BASE, 4);
    PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);
    
   altera_avalon_mutex_unlock(mutex_0);
    
    /* read five from fifo */
    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_2);
    altera_avalon_fifo_read_fifo(FIFO_0_IN_BASE, FIFO_0_OUT_CSR_BASE);
    altera_avalon_fifo_read_fifo(FIFO_0_IN_BASE, FIFO_0_OUT_CSR_BASE);
    altera_avalon_fifo_read_fifo(FIFO_0_IN_BASE, FIFO_0_OUT_CSR_BASE);
    altera_avalon_fifo_read_fifo(FIFO_0_IN_BASE, FIFO_0_OUT_CSR_BASE);
    PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_2);
    
    /* write 512 to shared memory */
    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_3);
    int *value = (unsigned char*) SHARED_ONCHIP_BASE + 1;
    for(i = 0 ; i < 512; i++){
        *(value + i) = i;
    }
    PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_3);
    
    /* Loop function */
    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_4);
    loopFunction(1024);
    PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_4);
    
    /* Print measurement result */
    perf_print_formatted_report(PERFORMANCE_COUNTER_0_BASE,
                                ALT_CPU_FREQ,
                                4,
                                "Write FIFO",
                                "Read FIFO",
                                "Write Shared Memory",
                                "Loop Function");
                                
                                
	while (TRUE) { 
	    
	    /* Check the switch */
	    checkSwitch();
	    /* check the keys */
	    keysValue = pollKeys();
	    
	    if(keysValue >=  0 ){
	        res = readFromFifo(keysValue, hexTab);
	        if( res == 0 ){
	            /* Display value on HEX */
	            printf(" read %d %d %d %d \n", hexTab[0],hexTab[1],hexTab[2],hexTab[3]);
	    
                IOWR(HEX3_HEX0_BASE, 0,sev_seg_decoder(hexTab[0] + hexTab[1] * 100));
                IOWR(HEX7_HEX4_BASE,0, sev_seg_decoder(hexTab[2] + hexTab[3] *100));
            }
            else {
                altera_avalon_mutex_lock(mutex_2, 1);
                readFromShared(keysValue, hexTab);
                IOWR(HEX3_HEX0_BASE, 0,sev_seg_decoder(hexTab[0] + hexTab[1] * 100));
                IOWR(HEX7_HEX4_BASE,0, sev_seg_decoder(hexTab[2] + hexTab[3] *100));
                altera_avalon_mutex_unlock(mutex_2);
            }
        }
        
        /* delay for the keys, and data to be filled */
        delay(20);
   
	}
	return 0;
}



/* Function to write to the shared memory */
int writeToShared(int cpu_id, int value, int offset){
    
    unsigned char* writeAddress;
    writeAddress = ((unsigned char*) SHARED_ONCHIP_BASE + 1 + offset);
    
    *writeAddress = value;
    
    /* set the cpu_id */
    return 0;
    
}

/* Check the switch periodically */
void checkSwitch(){

    int  value = 0;
    value = IORD(SWITCHES_BASE, 0);
    int led_value = 0;
    led_value = value;
    IOWR(LEDS_RED_BASE, 0 , led_value);
    
}

int pollKeys(){

    int keysValue = IORD(BUTTONS_BASE, 0);
    if(keysValue == 1){
        IOWR(LEDS_GREEN_BASE,0, LED_GREEN_0);
        return 0;
    }
    else if(keysValue == 2){
        IOWR(LEDS_GREEN_BASE, 0,LED_GREEN_2);
        return 1;
    }
    else if(keysValue == 4){
        IOWR(LEDS_GREEN_BASE, 0 ,LED_GREEN_4);
        return 2;
    }
    else if(keysValue == 8 ){
        IOWR(LEDS_GREEN_BASE, 0, LED_GREEN_6);
        return 3;
    }
    else {
        return -1;
    }
}

void loopFunction(int n){
    
    int i = 0, j = 0 ;
    int value;
    for(i = 0 ; i < n ; i++){
        for(j = 0 ; j < n; j++){
            value = i + j;
        }
    }
}


int readFromFifo(int fifoId, int *tab){

    int res = -1;
    int i = 0;
    if(fifoId == 0 ){
        altera_avalon_mutex_lock(mutex_0, 1);
        if(altera_avalon_fifo_read_level(FIFO_0_OUT_CSR_BASE) >= 4) {
            for(i = 0 ; i < 4 ; i++){
                tab[i] = altera_avalon_fifo_read_fifo(FIFO_0_OUT_BASE, FIFO_0_OUT_CSR_BASE);
            }
        }
            /* release mutex */
        altera_avalon_mutex_unlock(mutex_0);
        res = 0;
    
    }
    else if(fifoId == 1){
        altera_avalon_mutex_lock(mutex_1, 1);
            if(altera_avalon_fifo_read_level(FIFO_1_OUT_CSR_BASE) >= 4) {
                for(i = 0 ; i < 4 ; i++){
                    tab[i] = altera_avalon_fifo_read_fifo(FIFO_1_OUT_BASE, FIFO_1_OUT_CSR_BASE);
                }
            }
           altera_avalon_mutex_unlock(mutex_1); 
           res = 0; 
        
    }
    
    return res;
}
int readFromShared(int index, int *sharedAddress){

    /* data exchanged between core_3 and core_4 and core_0 start at SHARED_BASE + 2048
    */
    
    int res = 0;
    int i = 0;
    int offset = 0;
    if( index  == 2){
        offset = 0;
    }
    else if( index == 3 ){
        offset = 0 + 10;
    }
    
    unsigned char* value;
    value = (unsigned char*)( SHARED_ONCHIP_BASE + 1 + offset);
    for(i = 0 ; i < 4; i++){
        *(value + i) = *(sharedAddress + i);
        printf(" received data from cpu_%d : %d\n", index+1, *(value + i));
    }
    
    return res;
}

        
