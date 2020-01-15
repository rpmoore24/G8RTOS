#include "msp.h"
#include "BSP.h"
#include <stdint.h>
#include <stdlib.h>
#include <driverlib.h>
#include <G8RTOS_IPC.h>
#include <G8RTOS_Scheduler.h>
#include <G8RTOS_Semaphores.h>
#include <G8RTOS_Structures.h>
#include <stdio.h>
#include <LCD.h>
#include <stdint.h>
#include <stdbool.h>
#include "BackChannelUart.h"
#include "Game.h"
#include "time.h"

#define JOYSTICKFIFO 0

#define X 0
#define Y 1

uint16_t playerColors[] = {LCD_RED, LCD_BLUE, LCD_GREEN, LCD_YELLOW};
int8_t initialVeloX[] = {1, 0, -1, 0};
int8_t initialVeloY[] = {0, 1, 0, -1};
int16_t initialX[] = {100, 260, 260, 100};
int16_t initialY[] = {80, 80, 160, 160};
/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame()
{
    int i;
    initCC3100(0);

    client.IP_address = getLocalIP();
    client.acknowledge = false;
    client.ready = false;
    client.joined = true;
//    client.velocityX = -1;
//    client.velocityY = 0;

    // wait for server response
    while(!game.clients[game.activePlayers].acknowledge)
    {
        SendData(&client, HOST_IP_ADDR, sizeof(client));
        ReceiveData(&game, sizeof(game));

    }
    client.clientNumber = game.activePlayers;
    game.clients[client.clientNumber].acknowledge = false;

    client.joined = false;
    SendData(&client, HOST_IP_ADDR, sizeof(client));

    game.ready = false;
    while(!game.ready)
    {
        ReceiveData(&game, sizeof(game));
    }

    client.velocityX = game.players[client.clientNumber + 1].velocityX;
    client.velocityY = game.players[client.clientNumber + 1].velocityY;

    // show connection with LED
    if (client.clientNumber == 0)
    {
        P2->SEL1 &= ~BLUE_LED;
        P2->SEL0 &= ~BLUE_LED;
        P2->DIR |= BLUE_LED;
        P2->OUT = BLUE_LED;
    }
    else
    {
        P2->SEL1 &= ~GREEN_LED;
        P2->SEL0 &= ~GREEN_LED;
        P2->DIR |= GREEN_LED;
        P2->OUT = GREEN_LED;
    }

    //initialize semaphores
    G8RTOS_InitSemaphore(&Wifi, 1);
    G8RTOS_InitSemaphore(&LCD, 1);
    G8RTOS_InitSemaphore(&PLAYER, 1);

    InitBoardState();

    //add threads
    G8RTOS_AddThread(&IdleThread, 255, "idle");
    G8RTOS_AddThread(&DrawClient, 175, "drawobj");
    G8RTOS_AddThread(&ReadJoystickClient, 149, "rjc");
    G8RTOS_AddThread(&SendDataToHost, 149, "sendH");
    G8RTOS_AddThread(&ReceiveDataFromHost, 150, "recieveH");

    G8RTOS_KillSelf();

    while(1);
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost()
{
    int retval;
    while(1)
    {
        retval = -1;
        while(retval<0)
        {
            G8RTOS_WaitSemaphore(&Wifi);
            retval = ReceiveData(&game, sizeof(game));
            G8RTOS_SignalSemaphore(&Wifi);
        }
        if(game.gameDone)
        {
            G8RTOS_AddThread(&EndOfGameClient,10,"endgameC");
        }
        OS_Sleep(2);
    }
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{
    while(1)
    {
        G8RTOS_WaitSemaphore(&Wifi);
        SendData(&client,HOST_IP_ADDR,sizeof(client));
        G8RTOS_SignalSemaphore(&Wifi);
        OS_Sleep(2);
    }
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient(){
    int16_t x_coord;
    int16_t y_coord;

    while(1)
    {
        GetJoystickCoordinates(&x_coord, &y_coord);

        if (abs(x_coord) > 4000 || abs(y_coord) > 4000)
        {
            if (abs(x_coord) > abs(y_coord))
            {
                if (x_coord > 0)
                {
                    if (client.velocityX != 1)
                    {
                        client.velocityX = -1;
                        client.velocityY = 0;
                    }
                }
                else
                {
                     if (client.velocityX != -1)
                     {
                        client.velocityX = 1;
                        client.velocityY = 0;
                     }
                }
            }
            else if (abs(x_coord) < abs(y_coord))
            {
                if (y_coord > 0)
                {
                    if (client.velocityY != -1)
                    {
                        client.velocityY = 1;
                        client.velocityX = 0;
                    }
                }
                else
                {
                    if (client.velocityY != 1)
                    {
                        client.velocityY = -1;
                        client.velocityX = 0;
                    }
                }
            }

        }

        OS_Sleep(2);
    }
}

/*
 * End of game for the client
 */
void EndOfGameClient()
{
    int i;
    // Wait for all semaphores to be released
    G8RTOS_WaitSemaphore(&Wifi);
    G8RTOS_WaitSemaphore(&LCD);
    G8RTOS_WaitSemaphore(&PLAYER);
    G8RTOS_WaitSemaphore(&LED);

    // Kill all other threads
    G8RTOS_KillAllThreads();


    // Clear screen with winner's color
    for (i = 0; i < NUM_OF_PLAYERS_PLAYING; i++)
    {
        if (game.players[i].alive)
        {
            LCD_Clear(game.players[i].color);
            break;
        }
    }

    // Print message waiting for host to start new game
    LCD_Text(0, 0, "WAITING FOR HOST TO START NEXT GAME.", LCD_WHITE);

    client.acknowledge = true;
    game.clients[client.clientNumber].acknowledge = false;
    while(!game.clients[client.clientNumber].acknowledge)
    {
        SendData(&client, HOST_IP_ADDR,sizeof(client));
        ReceiveData(&game, sizeof(game));
    }
    game.clients[client.clientNumber].acknowledge = false;
    client.acknowledge = false;

    game.ready = false;
    while(!game.ready)
    {
        ReceiveData(&game, sizeof(game));
    }
    game.ready = false;

    InitBoardState();

    // Re-initialize semaphores
    G8RTOS_InitSemaphore(&Wifi, 1);
    G8RTOS_InitSemaphore(&LCD, 1);
    G8RTOS_InitSemaphore(&PLAYER,1);
    G8RTOS_InitSemaphore(&LED, 1);

    // Add all threads back
    G8RTOS_AddThread(&ReadJoystickClient, 149, "ReadJoystick");
    G8RTOS_AddThread(&SendDataToHost, 149, "SendToHost");
    G8RTOS_AddThread(&ReceiveDataFromHost, 150, "ReceiveFromHost");
    G8RTOS_AddThread(&DrawClient, 175, "Draw");
    G8RTOS_AddThread(&IdleThread, 255, "Idle");

    client.acknowledge = false;
    client.ready = false;
    client.joined = true;

    // Kill Self
    G8RTOS_KillSelf();
}

/*
 * Thread to draw all the objects in the game
 */
void DrawClient()
{
    GeneralPlayerInfo_t p;

    while(1)
    {
        for(int i = 0; i < NUM_OF_PLAYERS_PLAYING; i++)
        {
            p = game.players[i];
            LCD_DrawRectangle(p.positionX - PLAYER_WIDTH_D2, p.positionX + PLAYER_WIDTH_D2, p.positionY - PLAYER_WIDTH_D2, p.positionY + PLAYER_WIDTH_D2, p.color);
        }

        OS_Sleep(10);
    }
}

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
void CreateGame()
{
    uint32_t IP_check = 0;
    LCD_Text(70, 100, "Host", LCD_WHITE);
    LCD_Text(215, 100, "Client", LCD_WHITE);
    LCD_DrawRectangle(159, 161, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);
    while (!flag);
//    NVIC_DisableIRQ(PORT4_IRQn);

    //initialize players
    playerType player = GetPlayerRole();
//    player = Host;
    if(player == Client){
        void (*join_ptr)(void) = &JoinGame;
        G8RTOS_AddThread(join_ptr, 255, "join");
        G8RTOS_KillSelf();
    }
    else{
//        establish connection
        initCC3100(1);
        _u32 IP = getLocalIP();

        GeneralPlayerInfo_t * p;
        //initialize players and board
        for (int i = 0; i < NUM_OF_PLAYERS_PLAYING; i++)
        {
            p = &game.players[i];
            p->alive = true;
            p->color = playerColors[i];
            p->velocityX = initialVeloX[i];
            p->velocityY = initialVeloY[i];
            p->positionX = initialX[i];
            p->positionY = initialY[i];
            game.overallScores[i] = 0;
        }
        game.gameDone = false;
        game.alive = NUM_OF_PLAYERS_PLAYING;

        //acknowledge clients
        for(int i = 0; i < NUM_OF_CLIENTS; i++)
        {
            clients[i].joined = false;
            //Receive data from client
            while(!clients[i].joined)
            {
                ReceiveData(&clients[i], sizeof(clients[i]));
            }

            game.clients[i] = clients[i];
            game.clients[i].acknowledge = true;


            SendData(&game, game.clients[i].IP_address, sizeof(game));
            SendData(&game, game.clients[i].IP_address, sizeof(game));
            game.activePlayers++;

            while(clients[i].joined)
            {
                ReceiveData(&clients[i], sizeof(clients[i]));
            }
        }

        game.ready = true;
        for(int i = 0; i < NUM_OF_CLIENTS; i++)
        {
            SendData(&game, game.clients[i].IP_address, sizeof(game));
            SendData(&game, game.clients[i].IP_address, sizeof(game));
        }

        P2->SEL1 &= ~RED_LED;
        P2->SEL0 &= ~RED_LED;
        P2->DIR |= RED_LED;
        P2->OUT = RED_LED;

        InitBoardState();

        //add threads
        G8RTOS_AddThread(&IdleThread, 255, "idle");
        G8RTOS_AddThread(&DrawHost, 175, "drawobj");
        G8RTOS_AddThread(&ReadJoystickHost, 200, "rjh");
        G8RTOS_AddThread(&SendDataToClient, 150, "sendC");
        G8RTOS_AddThread(&ReceiveDataFromClient, 150, "recieveC");

    }

    //kill this thread
    G8RTOS_KillSelf();

}

/*
 * Thread that sends game state to client
 */
void SendDataToClient(){
    while(1)
    {
        G8RTOS_WaitSemaphore(&Wifi);
        for(int i = 0; i < NUM_OF_PLAYERS_PLAYING - 1; i++){
            SendData(&game, game.clients[i].IP_address, sizeof(game));
        }
        G8RTOS_SignalSemaphore(&Wifi);
        OS_Sleep(5);
    }
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{
    int retval;
    while(1)
    {
        retval=-1;
        client.clientNumber = -1;
        for(int i = 0; i < NUM_OF_CLIENTS; i++)
        {
            while(!client.clientNumber == i)
            {
                G8RTOS_WaitSemaphore(&Wifi);
                retval = ReceiveData(&client,sizeof(client));
                G8RTOS_SignalSemaphore(&Wifi);
            }
            game.clients[client.clientNumber] = client;
            game.players[client.clientNumber+1].velocityX = client.velocityX;
            game.players[client.clientNumber+1].velocityY = client.velocityY;
        }

        OS_Sleep(2);
    }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost()
{
    int16_t x_coord;
    int16_t y_coord;

    while(1)
    {
        GetJoystickCoordinates(&x_coord, &y_coord);

        if (abs(x_coord) > 4000 || abs(y_coord) > 4000)
        {
            if (abs(x_coord) > abs(y_coord))
            {
                if (x_coord > 0)
                {
                    if(game.players[0].velocityX != 1)
                    {
                        game.players[0].velocityX = -1;
                        game.players[0].velocityY = 0;
                    }
                }
                else
                {
                    if(game.players[0].velocityX != -1)
                    {
                        game.players[0].velocityX = 1;
                        game.players[0].velocityY = 0;
                    }
                }
            }
            else if (abs(x_coord) < abs(y_coord))
            {
                if (y_coord > 0)
                {
                    if(game.players[0].velocityY != -1)
                    {
                        game.players[0].velocityY = 1;
                        game.players[0].velocityX = 0;
                    }
                }
                else
                {
                    if(game.players[0].velocityY != 1)
                    {
                        game.players[0].velocityY = -1;
                        game.players[0].velocityX = 0;
                    }
                }
            }

        }

        OS_Sleep(10);
    }
}

/*
 * End of game for the host
 */
void EndOfGameHost()
{
    // Wait for the semaphores to be released
        G8RTOS_WaitSemaphore(&LCD);
        G8RTOS_WaitSemaphore(&Wifi);
        G8RTOS_WaitSemaphore(&PLAYER);


        // Kill all other threads
        G8RTOS_KillAllThreads();

        // Re-initialize semaphores
       G8RTOS_InitSemaphore(&Wifi, 1);
       G8RTOS_InitSemaphore(&LCD, 1);
       G8RTOS_InitSemaphore(&PLAYER,1);

    game.gameDone = true;
    game.ready = false;
    client.acknowledge = false;
    client.clientNumber = -1;
    GeneralPlayerInfo_t * p;

    for(int i = 0; i < NUM_OF_PLAYERS_PLAYING - 1; i++){
        game.clients[i].acknowledge = false;
        while(client.clientNumber != i && client.acknowledge != true)
        {
            SendData(&game,game.clients[i].IP_address,sizeof(game));
            ReceiveData(&client,sizeof(client));
        }
        game.clients[i] = client;
        SendData(&game,game.clients[i].IP_address,sizeof(game));
    }

    for(int i = 0; i<NUM_OF_PLAYERS_PLAYING; i++)
    {
        if(game.players[i].alive)
        {
            LCD_Clear(game.players[i].color);
            game.overallScores[i]++;
            break;
        }
    }

    // Print message waiting for host to start new game
    LCD_Text(0, 0, "WAITING FOR HOST TO START NEXT GAME.", LCD_WHITE);

    // Add aperiodic event that waits for host's button press

    while(!restart);
    restart = 0;

    // Once host starts new game, send notification to client, reinitialize game/objects, add back all threads, and kill self;
    for (int i = 0; i < NUM_OF_PLAYERS_PLAYING; i++)
    {
        p = &game.players[i];
        p->alive = true;
        p->color = playerColors[i];
        p->velocityX = initialVeloX[i];
        p->velocityY = initialVeloY[i];
        p->positionX = initialX[i];
        p->positionY = initialY[i];
    }

    game.gameDone = false;
    game.alive = NUM_OF_PLAYERS_PLAYING;

    game.ready = true;
    for(int i = 0; i < NUM_OF_CLIENTS; i++)
    {
        SendData(&game, game.clients[i].IP_address, sizeof(game));
        SendData(&game, game.clients[i].IP_address, sizeof(game));
    }

    InitBoardState();

    G8RTOS_AddThread(&IdleThread, 255, "idle");
    G8RTOS_AddThread(&DrawHost, 175, "drawobj");
    G8RTOS_AddThread(&ReadJoystickHost, 200, "rjh");
    G8RTOS_AddThread(&SendDataToClient, 150, "sendC");
    G8RTOS_AddThread(&ReceiveDataFromClient, 150, "recieveC");

    G8RTOS_KillSelf();
}

void DrawHost()
{
    GeneralPlayerInfo_t p;
    while(1)
    {
        for(int i = 0; i < NUM_OF_PLAYERS_PLAYING; i++)
        {
            if(game.players[i].alive)
            {
                game.players[i].positionX += game.players[i].velocityX;
                game.players[i].positionY += game.players[i].velocityY;
                p = game.players[i];
                if (p.velocityX == -1 || p.velocityY == -1)
                {
                    if (LCD_ReadPixelColor(p.positionX - PLAYER_WIDTH_D2, p.positionY - PLAYER_WIDTH_D2) != BACK_COLOR)
                    {
                        game.players[i].alive = false;
                        game.alive--;
                    }
                }
                else
                {
                    if (LCD_ReadPixelColor(p.positionX + PLAYER_WIDTH_D2, p.positionY + PLAYER_WIDTH_D2) != BACK_COLOR)
                    {
                        game.players[i].alive = false;
                        game.alive--;
                    }
                }

                if (p.positionX < ARENA_MIN_X || p.positionX > ARENA_MAX_X || p.positionY < ARENA_MIN_Y || p.positionY > ARENA_MAX_Y)
                {
                    game.players[i].alive = false;
                    game.alive--;
                }
                if(game.players[i].alive)
                    LCD_DrawRectangle(p.positionX - PLAYER_WIDTH_D2, p.positionX + PLAYER_WIDTH_D2, p.positionY - PLAYER_WIDTH_D2, p.positionY + PLAYER_WIDTH_D2, p.color);
            }
        }

        if(game.alive < 2)
        {
            game.gameDone = true;
            G8RTOS_AddThread(&EndOfGameHost,50,"endgame");
            G8RTOS_KillSelf();
        }
        OS_Sleep(10);
    }
}

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread(){
    while(1);
}


/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole(){

    if(tp.x > 160){
        return Client;
    }
    else{
        return Host;
    }
}

/*
 * Initializes and prints initial game state
 */
void InitBoardState(){

    G8RTOS_WaitSemaphore(&LCD);
    LCD_Clear(LCD_BLACK);
    LCD_DrawRectangle(ARENA_MIN_X-1, ARENA_MIN_X, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);
    G8RTOS_SignalSemaphore(&LCD);
    updateOverallScores();
}

void updateOverallScores(){
    G8RTOS_WaitSemaphore(&LCD);
    char c[2];
    c[0] = 0;
    c[1] = 0;
    uint8_t val = 0;
    playerColor col;

    uint16_t yPos = 0;
    for(int i = 0; i < NUM_OF_PLAYERS_PLAYING; i++){
        val = game.overallScores[i];
        c[0] = (val/10) + 48;
        c[1] =  (val%10) + 48;
        if(i == 0){
            col = PLAYER_RED;
        }
        else if(i == 1){
            col = PLAYER_BLUE;
        }
        else{
            col = PLAYER_GREEN;
        }
        LCD_Text(5, yPos, c, col);
        yPos += 25;
    }

    G8RTOS_SignalSemaphore(&LCD);
}
/*********************************************** Public Functions *********************************************************************/
