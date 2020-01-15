/*
 * G8RTOS_IPC.c
 *
 *  Created on: Jan 10, 2017
 *      Author: Daniel Gonzalez
 */
#include <G8RTOS/G8RTOS_IPC.h>
#include <G8RTOS/G8RTOS_Semaphores.h>
#include <G8RTOS/G8RTOS_Structures.h>
#include <stdint.h>
#include "msp.h"

/*********************************************** Defines ******************************************************************************/

#define FIFOSIZE 16
#define MAX_NUMBER_OF_FIFOS 4

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/*
 * FIFO struct will hold
 *  - buffer
 *  - head
 *  - tail
 *  - lost data
 *  - current size
 *  - mutex
 */

/* Create FIFO struct here */
typedef struct fifo fifo_t;

struct fifo{
    int32_t buffer[FIFOSIZE];
    int32_t *Head;
    int32_t *Tail;
    uint32_t LostData;
    semaphore_t CurrentSize;
    semaphore_t Mutex;
};
/* Array of FIFOS */
static fifo_t FIFOs[4];

/*********************************************** Data Structures Used *****************************************************************/

/*
 * Initializes FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex)
{
    /* Implement this */
    if(FIFOIndex < MAX_NUMBER_OF_FIFOS)
    {
        FIFOs[FIFOIndex].Head = &FIFOs[FIFOIndex].buffer[0];
        FIFOs[FIFOIndex].Tail = &FIFOs[FIFOIndex].buffer[0];
        FIFOs[FIFOIndex].LostData = 0;
        G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].CurrentSize,0);
        G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].Mutex,1);
    }
    return 1;
}

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param: "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
int32_t readFIFO(uint32_t FIFOChoice)
{
    /* Implement this */
    //Wait til Fifo is available and has data
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].CurrentSize);
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].Mutex);

    int32_t value;
    value = *FIFOs[FIFOChoice].Head;
    if(FIFOs[FIFOChoice].Head == &FIFOs[FIFOChoice].buffer[15])
    {
        FIFOs[FIFOChoice].Head = &FIFOs[FIFOChoice].buffer[0];
    }
    else
    {
        FIFOs[FIFOChoice].Head++;
    }
    //release for other threads
    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].Mutex);

    return value;
}

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if ncessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFOChoice, uint32_t Data)
{
    /* Implement this */
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].Mutex);

    if(FIFOs[FIFOChoice].CurrentSize > FIFOSIZE-1)
    {
        FIFOs[FIFOChoice].LostData++;
        *FIFOs[FIFOChoice].Head = Data;
        if(FIFOs[FIFOChoice].Head == &FIFOs[FIFOChoice].buffer[15])
        {
            FIFOs[FIFOChoice].Head = &FIFOs[FIFOChoice].buffer[0];
        }
        else
        {
            FIFOs[FIFOChoice].Head++;
        }
    }
    else
    {
        *FIFOs[FIFOChoice].Tail = Data;
        G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].CurrentSize);
    }

    if(FIFOs[FIFOChoice].Tail == &FIFOs[FIFOChoice].buffer[15])
    {
        FIFOs[FIFOChoice].Tail = &FIFOs[FIFOChoice].buffer[0];
    }
    else
    {
        FIFOs[FIFOChoice].Tail++;
    }

    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].Mutex);
    return Data;
}

