#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "io.h"
#include "altera_avalon_performance_counter.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_fifo_util.h"
#include "altera_avalon_fifo_regs.h"
#include "altera_avalon_mutex.h"




#define TRUE 1

extern void delay (int millisec);



/* Cpu 1 writes pairs values from 0 to 99 to fifo 0 */

int main()
{
    int i = 0;
    int value = 0;
    printf("Hello from cpu_1!\n");
    altera_mutex_dev *mutex_0 = altera_avalon_mutex_open(MUTEX_0_NAME);
    
  
    while (TRUE) {
        altera_avalon_mutex_lock(mutex_0, 1);
        /* It would be nice to generate, random number but don't know if we have the library */
        for(i = 0 ; i < 5 ; i++){
            altera_avalon_fifo_write_fifo(FIFO_0_IN_BASE, FIFO_0_IN_CSR_BASE, value);
            value += 2;
        }
        altera_avalon_mutex_unlock(mutex_0);
        
        delay(40);
        value += 2;
        if(value >= 100){
            /* reset the value because can not display a 3 digit number on one hex */
            value = 0;
        }
    }
    return 0;
}