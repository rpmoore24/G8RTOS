#include <G8RTOS_IPC.h>
#include <G8RTOS_Scheduler.h>
#include <G8RTOS_Semaphores.h>
#include <G8RTOS_Structures.h>
#include "msp.h"
#include "BSP.h"
#include "Game.h"
#include <stdint.h>
#include <stdlib.h>
#include <driverlib.h>
#include <stdio.h>
#include <LCD.h>
#include <stdint.h>
#include <stdbool.h>
#include "BackChannelUart.h"


/**
 * main.c
 */

void PORT4_IRQHandler(void){
    uint32_t t = GPIO_getEnabledInterruptStatus(GPIO_PORT_P4);
       __delay_cycles(48000);
       if(GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0) == GPIO_INPUT_PIN_LOW){
           GPIO_clearInterruptFlag(GPIO_PORT_P4, t);
           tp = TP_ReadXY();
           flag = 1;
       }
       else if((P4IN | ~BIT4) == ~BIT4){
           GPIO_clearInterruptFlag(GPIO_PORT_P4, t);
           restart = 1;
       }
}

void main(void)
{
    P4 -> DIR &= ~BIT4;
    P4 -> IFG &= ~BIT4;
    P4 -> IE |= BIT4;
    P4 -> IES |= BIT4;
    P4 -> REN |= BIT4;
    P4 -> OUT |= BIT4;

    G8RTOS_Init();

    G8RTOS_InitSemaphore(&Wifi,1);
    G8RTOS_InitSemaphore(&LCD, 1);
    G8RTOS_InitSemaphore(&LED, 1);
    G8RTOS_InitSemaphore(&PLAYER,1);


    G8RTOS_AddThread(&CreateGame, 100, "create");

    G8RTOS_Launch();

}
