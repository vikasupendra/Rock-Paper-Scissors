#pragma once

#include <string.h>   
#include <stdlib.h> 
#include <unistd.h>   
#include <arpa/inet.h>    
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/select.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <sys/time.h> 
#include <cstring>
#include <iostream>
#include <vector>
#include <thread>
#include <windows.h>

using namespace std;

enum class GAME_CMD  {CMD_INVALID, INIT, PLAY, END};
enum class PLAYER_ID {PID_INVALID, PID1 = 1, PID2 = 2, PID_NULL};
enum class MYPLAY    {PLAY_INVALID, ROCK, PAPER, SCISSORS};

using UINT8  = unsigned __int8;
using UINT16 = unsigned __int16;
using UINT32 = unsigned __int32;

#define    LOCALPORT       8880    // server listening port
#define    PLAYER1_PORT    8881    // player 1 binds to this port
#define    PLAYER2_PORT    8882    // player 2 binds to this port 
#define    MAX_GAME_COUNT  100     // simulate for 100 games
#define    MAX_CLIENTS     2       // max number of clients/connections

// COMMAND sent ONLY from server to clients
struct Command
{
  GAME_CMD   cmd;             // Command - INIT, PLAY or END
  PLAYER_ID  PlayerID;        // player ID - 1 or 2
  int        CurrGameID;      // current game identifier
  void       *PlayHistory;    // Pointer to game history so far - OWNED by main thread but accessible for player threads 
  
  Command():PlayerID(PLAYER_ID::PID_INVALID)
  {
    cmd = GAME_CMD::CMD_INVALID;
  }
  
  Command(PLAYER_ID pID):PlayerID(pID)
  {
    cmd = GAME_CMD::INIT;
  }
};

using COMMAND = struct Command;

struct  GameResult
{
  PLAYER_ID  Winner;        
  int        Round;
  MYPLAY     P1Play;
  MYPLAY     P2Play;
};

struct PlayerResponse
{
  PLAYER_ID   pID;
  MYPLAY      ePlay;
  int         currGameID;
};

class Player
{
  PLAYER_ID PlayerID;
  int       sockfd;
  int       currGameID;
  
public:
  Player(PLAYER_ID pID):PlayerID(pID)
  {
    currGameID = 1;
  }

  ~Player()
  {
    SocketClose();    
  }

  void    SetSocket(int fd);
  MYPLAY  ApplyPlayer1Strategy();
  MYPLAY  ApplyPlayer2Strategy();
  MYPLAY  ApplyPlayer2Strategy(const std::vector<GameResult> &GameHistory);
  void    PlayerPlay(const COMMAND &CurrCmd);
  inline bool IsGameIDValid(int GameID);
  inline void SetNextGameID();

  bool    ConnectPlayer();
  void    RecvGameCommand(COMMAND *currCmd);
  void    SendPlayerReadyACK();
  void    SendPlayerGamePlay(const PlayerResponse *resp);
  void    SocketClose();
};

void Player1Thread();
void Player2Thread();

PLAYER_ID DeclareWinner(const GameResult &res);