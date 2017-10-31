#include "players.h"
#include "json/json.h"

MYPLAY RandomizePlay()
{
  int min = (int)(MYPLAY::ROCK), max = (int)(MYPLAY::SCISSORS);

  int range = max - min + 1;
  int play = rand() % range + min;

  cout << "player played --> " << play << endl;
  return (MYPLAY)play;
}

void Player::SetSocket(int fd)
{
  sockfd = fd;
}

void Player::PlayerPlay(const COMMAND &CurrCmd)
{
  PlayerResponse resp;

  memset(&resp, 0, sizeof(PlayerResponse));

  int gameID = CurrCmd.CurrGameID;

  if(IsGameIDValid(gameID))                         // Check if expected gameID is correct
  {
    SetNextGameID();                                // Set next gameID that player is expecting

    /******** Apply player STRATEGY *******/
    if(PLAYER_ID::PID1 == PlayerID)
    {
      resp.ePlay = ApplyPlayer1Strategy();          // For player 1, we randomize between ROCK, PAPER and SCISSORS
    }
    else
    {
      std::vector<GameResult> *pHistory = (std::vector<GameResult> *)(CurrCmd.PlayHistory);

      resp.ePlay = ApplyPlayer2Strategy(*pHistory);  // For player 2, use player 1's history of game plays and decide on current play
    }

    resp.currGameID = gameID;
    resp.pID = PlayerID;

    cout << "Sending resp from PID --> " << (int)(resp.pID) << " | gameID --> " << resp.currGameID << " | game play --> " << (int)(resp.ePlay) << endl;
    
    /********  SEND this game's play to main thread ********/
    SendPlayerGamePlay(&resp);
  }
  else
  {
    cout << "gameID recvd is INVALID | MUST NOT ENTER this condition" << endl;
  }
}

MYPLAY Player::ApplyPlayer1Strategy()
{
  // randomize ROCK, PAPER or SCISSORS for player1
  return RandomizePlay();
}

MYPLAY Player::ApplyPlayer2Strategy()
{
  // randomize ROCK, PAPER or SCISSORS for player2 ONLY for the first move
  return RandomizePlay();
}

MYPLAY Player::ApplyPlayer2Strategy(const std::vector<GameResult> &GameHistory)
{
  // check history of all previous player1 moves and choose player1's last move as player 2 move
  if(!GameHistory.empty())
  {
    GameResult lastP1Move = GameHistory.back();
    cout << "Last p1 move --> " << (int)(lastP1Move.P1Play);
    return lastP1Move.P1Play;
  }
  else
  {
    return ApplyPlayer2Strategy();
  }
}

inline bool Player::IsGameIDValid(int GameID)
{
  return (currGameID == GameID) ? true : false;
}

inline void Player::SetNextGameID()
{
  currGameID++;
}

bool Player::ConnectPlayer()
{
  int port = PLAYER1_PORT;
  int fd = 0;
  int opt = true;

  if(PLAYER_ID::PID2 == PlayerID)
  {
    port = PLAYER2_PORT;
  }
  
  struct sockaddr_in serv_addr;
  struct hostent *server;
  socklen_t sin_size = sizeof(struct sockaddr);

  if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    cout << "Socket error" << endl;
    return false;
  }

  SetSocket(fd);

  server = gethostbyname("localhost");
  if(server == NULL)
  {
    cout << "Invalid host!" << endl;
    SocketClose();
    return false;
  }

  std::memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
  {
	  cout << "bind failed" << endl;
    SocketClose();
	  return false;
  }

  std::memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(LOCALPORT);

  if(connect(fd, (struct sockaddr*)&serv_addr, sin_size) == -1)
  {
    std::cout << "Socket Connect failed" << endl;
    SocketClose();
    return false;
  }

  return true;
}

void Player::RecvGameCommand(COMMAND *currCmd)
{
  char *pBuf = (char *)currCmd;
  bool err = true;
  int val = 0;

  if((val = read(sockfd, pBuf, sizeof(COMMAND))) < 0) 
  {
    cout << "error reading message" << endl;
    return;
  }

  cout << "received cmd --> " << (int)(currCmd->cmd) << " | for playerID --> " << (int)(currCmd->PlayerID) << endl;
}

void Player::SendPlayerReadyACK()
{
  string message = "READY from player ";
  message.append(to_string((int)(PlayerID)));

  if(send(sockfd, message.c_str(), message.length(), 0) < message.length())
  {  
    cout << "send error " << endl;  
  }
}

void Player::SendPlayerGamePlay(const PlayerResponse *resp)
{
  if(send(sockfd, resp, sizeof(PlayerResponse), 0) < sizeof(PlayerResponse))
  {  
    cout << "send error " << endl;  
  }
}

void Player::SocketClose()
{
  cout << "Player: " << (int)(PlayerID) << " | Closing socket --> " << sockfd << endl;
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
}

void Player1Thread()
{
  Player  p1Obj(PLAYER_ID::PID1);
  COMMAND CurrCmd;

  sleep(0.5);
  if(!p1Obj.ConnectPlayer())
    return;

  cout << "Client connected !!" << endl;

  // Send notification to main thread that players are READY
  p1Obj.SendPlayerReadyACK();

  while(1)
  {
    memset(&CurrCmd, 0, sizeof(COMMAND));

    /****** RECV play command from main thread ******/
    p1Obj.RecvGameCommand(&CurrCmd);

    /****** PLAY current GAME *****/
    p1Obj.PlayerPlay(CurrCmd);

    /****** END GAME *****/
    if(CurrCmd.CurrGameID >= MAX_GAME_COUNT)
    {
      cout << "thread p1:: GAME END !!!" << endl;
      break;
    }
  }

  // player object destructor will close the socket
}

void Player2Thread()
{
  Player  p2Obj(PLAYER_ID::PID2);
  COMMAND CurrCmd;

  sleep(0.5);
  if(!p2Obj.ConnectPlayer())
    return;

  cout << "Client connected !!" << endl;

  // Send notification to main thread that players are READY
  p2Obj.SendPlayerReadyACK();
  
  while(1)
  {
    memset(&CurrCmd, 0, sizeof(COMMAND));

    /****** RECV play command from main thread ******/
    p2Obj.RecvGameCommand(&CurrCmd);

    /****** PLAY current GAME *****/
    p2Obj.PlayerPlay(CurrCmd);

    /****** END GAME ******/
    if(CurrCmd.CurrGameID >= MAX_GAME_COUNT)
    {
      cout << "thread p2:: GAME END !!!" << endl;
      break;
    }
  }

  // player object destructor will close the socket
}

PLAYER_ID DeclareWinner(const GameResult &res)
{
  if(res.P1Play == res.P2Play)
    return PLAYER_ID::PID_NULL;

  switch(res.P1Play)
  {
    case MYPLAY::ROCK:
    {
      if(MYPLAY::PAPER == res.P2Play)
        return PLAYER_ID::PID2;               // paper covers rock, p2 wins
      else
        return PLAYER_ID::PID1;               // rock smashes scissors, p1 wins
      break;
    }

    case MYPLAY::PAPER:
    {
      if(MYPLAY::SCISSORS == res.P2Play)
        return PLAYER_ID::PID2;               // scissors cut paper, p2 wins
      else
        return PLAYER_ID::PID1;               // paper covers rock, p1 wins
      break;
    }

    case MYPLAY::SCISSORS:
    {
      if(MYPLAY::ROCK == res.P2Play)
        return PLAYER_ID::PID2;               // rock smashes scissors, p2 wins 
      else
        return PLAYER_ID::PID1;               // scissors cut paper, p1 wins
      break;
    }

    default:
    {
      return PLAYER_ID::PID_INVALID;
    }
  }
}