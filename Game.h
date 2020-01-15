/*
 * Game.h
 *
 *  Created on: Feb 27, 2017
 *      Author: danny
 */

#ifndef GAME_H_
#define GAME_H_

/*********************************************** Includes ********************************************************************/
#include <stdint.h>
#include "cc3100_usage.h"
#include "G8RTOS.h"
#include "LCD.h"
/*********************************************** Includes ********************************************************************/

/*********************************************** Externs ********************************************************************/

/* Semaphores here */
semaphore_t LCD;
semaphore_t LED;
semaphore_t PLAYER;
semaphore_t sensor;
semaphore_t Wifi;


bool HC;
uint8_t *p1_score;
uint8_t *p2_score;
int flag;
Point tp;
int restart;


/*********************************************** Externs ********************************************************************/

/*********************************************** Global Defines ********************************************************************/
#define MAX_NUM_OF_PLAYERS  4

// This game can actually be played with 4 players... a little bit more challenging, but doable!
#define NUM_OF_PLAYERS_PLAYING      3
#define NUM_OF_CLIENTS              (NUM_OF_PLAYERS_PLAYING - 1)

/* Size of game arena */
#define ARENA_MIN_X                  30
#define ARENA_MAX_X                  320
#define ARENA_MIN_Y                  0
#define ARENA_MAX_Y                  240

/* Scoreboard */
#define SCOREBOARD_X                 5
#define SCOREBOARD_Y_1               5
#define SCOREBOARD_Y_2               20
#define SCOREBOARD_Y_3               35
#define SCOREBOARD_Y_4               50

/* Size of objects */
#define PLAYER_WIDTH                  4
#define PLAYER_WIDTH_D2               (PLAYER_WIDTH >> 1)
#define TOKEN_WIDTH                   4
#define TOKEN_WIDTH_D2                (TOKEN_WIDTH >> 1)

/* Background color - Black */
#define BACK_COLOR                   LCD_BLACK

/* Offset for printing player to avoid blips from left behind ball */
#define PRINT_OFFSET                10

/* Used as status LEDs for Wi-Fi */
#define BLUE_LED BIT2
#define GREEN_LED BIT1
#define RED_LED BIT0

/* Enums for player colors */
typedef enum
{
    TOKEN = LCD_YELLOW,
    PLAYER_RED = LCD_RED,
    PLAYER_BLUE = LCD_BLUE,
    PLAYER_MAGENTA = LCD_MAGENTA,
    PLAYER_GREEN = LCD_GREEN
}playerColor;

/* Enums for player numbers */
typedef enum
{
    ONE = 0,
    TWO = 1,
    THREE = 2
}playerNumber;

/*********************************************** Global Defines ********************************************************************/

/*********************************************** Data Structures ********************************************************************/
/*********************************************** Data Structures ********************************************************************/
#pragma pack ( push, 1)
/*
 * Struct to be sent from the client to the host
 */
typedef struct
{
    uint32_t IP_address;
    int8_t velocityX;
    int8_t velocityY;
    bool ready;
    bool joined;
    bool acknowledge;
    int8_t clientNumber;
} SpecificPlayerInfo_t;

/*
 * General player info to be used by both host and client
 * Client responsible for translation
 */
typedef struct
{
    int8_t velocityX;
    int8_t velocityY;
    int16_t positionX;
    int16_t positionY;
    playerNumber playerNumber;
    uint16_t color;
    bool alive;
} GeneralPlayerInfo_t;

/*
 * Struct to be sent from the host to the client
 */
typedef struct
{
    SpecificPlayerInfo_t clients[NUM_OF_CLIENTS];
    GeneralPlayerInfo_t players[NUM_OF_PLAYERS_PLAYING];
    bool winner;
    bool gameDone;
    bool ready;
    uint8_t alive;
    uint8_t overallScores[NUM_OF_PLAYERS_PLAYING];
    uint8_t activePlayers;

} GameState_t;
#pragma pack ( pop )

/*********************************************** Data Structures ********************************************************************/

GameState_t game;
SpecificPlayerInfo_t client;
SpecificPlayerInfo_t clients[NUM_OF_CLIENTS];

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame();

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost();

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost();

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient();

/*
 * End of game for the client
 */
void EndOfGameClient();

/*
 * Thread to draw all the objects in the game
 */
void DrawClient();

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame();

/*
 * Thread that sends game state to client
 */
void SendDataToClient();

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient();

/*
 * Generate Ball thread
 */
void GenerateBall();

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost();

/*
 * Thread to move a single ball
 */
void MoveBall();

/*
 * End of game for the host
 */
void EndOfGameHost();

/*
 * Thread to draw all the objects in the game
 */
void DrawHost();

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread();

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs();

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole();

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player);


/*
 * Initializes and prints initial game state
 */
void InitBoardState();

void updateOverallScores();

/*********************************************** Public Functions *********************************************************************/


#endif /* GAME_H_ */
