#include "players.h"
#include "JSONUtils.h"

using namespace std;

fd_set readfds;
int max_sd;
int client_socket[2];

// returns list of client sockfds + server sockfd
std::vector<int> CreateServer()
{ 
  int opt = true;  
  int server_socket, addrlen, new_socket, activity, sd, count = 0, valread = 0;
  struct sockaddr_in address;  
  std::vector<int> sockFDList;     
  char data[1024];
  
  //initialise all client_sockets
  for (int i = 0; i < MAX_CLIENTS; i++)  
  {  
    client_socket[i] = 0;  
  }  
        
  //create a server socket 
  if((server_socket = socket(AF_INET, SOCK_STREAM , 0)) == 0)  
  {  
    cout << "socket failed" << endl;
    close(server_socket);
    return sockFDList;
  }  
  
  //set master socket to allow multiple connections 
  if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )  
  {  
    cout << "setsockopt error" << endl;  
    close(server_socket);
    return sockFDList;
  } 
  
  //type of socket created 
  address.sin_family = AF_INET;  
  address.sin_addr.s_addr = INADDR_ANY;  
  address.sin_port = htons(LOCALPORT);  
      
  // bind the socket to localhost port 8880
  if(bind(server_socket, (struct sockaddr *)&address, sizeof(address))<0)  
  {  
    cout << "bind failed" << endl;  
    close(server_socket);
    return sockFDList;
  }  
  
  sockFDList.push_back(server_socket);    // add server socket to list of sock fds
      
  // maximum of 2 pending connections for the master socket 
  if (listen(server_socket, 2) < 0)  
  {  
    cout << "listen error" << endl;  
    close(server_socket);
    return sockFDList;  
  }  
      
  //accept the incoming connection 
  addrlen = sizeof(address);  
  cout << "Waiting for client connections ..." << endl;  

  while(1)  
  {  
    //clear the read set 
    FD_ZERO(&readfds);  

    //add server socket to set 
    FD_SET(server_socket, &readfds);  
    max_sd = server_socket;  

    //add child sockets to set 
    for (int i = 0 ;i < MAX_CLIENTS; i++)  
    {  
      //socket descriptor 
      sd = client_socket[i];  

      //if valid socket descriptor then add to read list 
      if(sd > 0)  
        FD_SET(sd , &readfds);  

      //highest file descriptor number for select call 
      if(sd > max_sd)  
        max_sd = sd;  
    }  

    //wait for an activity on one of the sockets , timeout is NULL ,so wait indefinitely 
    activity = select(max_sd + 1 , &readfds , NULL , NULL , NULL);  
  
    if((activity < 0) && (errno != EINTR))  
    {  
      cout << "select error" << endl;  
    }  
          
    // If there is some activity on the server socket, then its an incoming connection 
    if (FD_ISSET(server_socket, &readfds))  
    {  
      if ((new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)  
      {  
        cout << "accept ERROR" << endl;  
        close(server_socket);
        return sockFDList;
      }

      cout << "New connection , socket fd --> " << new_socket << " | port --> " << ntohs(address.sin_port) << endl;  

      UINT16 clientPort = ntohs(address.sin_port);
      if(clientPort == PLAYER1_PORT)
      {
        client_socket[0] = new_socket;     // used for sending data later client_socket[0] is for p1
      }
      else if(clientPort == PLAYER2_PORT)
      {
        client_socket[1] = new_socket;     // used for sending data later client_socket[1] is for p2
      }

      sockFDList.push_back(new_socket);    // add to list of sock fds
    }
    else
    {
      for (int i = 0; i < MAX_CLIENTS; i++)  
      {  
        int sd = client_socket[i];  
            
        if (FD_ISSET(sd, &readfds))  
        {  
          //Check if it was for closing , and also read the incoming message 
          if((valread = recv(sd, &data, 1024, 0)) <= 0)  
          {  
             cout << "Host disconnected , port --> " << endl;
             close(sd);  
             client_socket[i] = 0;  
          }  
          else
          {  
            data[valread] = '\0';

            string buffer(data);
            cout << "server receieved ACK from client --> " << buffer << endl;
            if(std::string::npos != buffer.find("1"))
              count++;

            else if (std::string::npos != buffer.find("2"))
              count++;
             
            if(count == MAX_CLIENTS)
            {
              cout << "clients ACK'd" << endl;
              return sockFDList;
            }
          }  
        }  
      }
    }
  }    
}

void GamePlay(const std::vector<int> &sockFDList)
{
  std::vector<Json::Value> JSONGameHistory;
  std::vector<GameResult> *GameHistory = new std::vector<GameResult>;
  GameResult     currGameRes;
  COMMAND        currCmd;
  PlayerResponse resp;
  int  valread = 0, sd = 0, activity, gameID = 1, count = 0, gameCnt = 0;
  
  while(1)
  {
    memset(&currCmd, 0, sizeof(COMMAND));
    memset(&resp, 0, sizeof(PlayerResponse));

    //clear the socket set 
    FD_ZERO(&readfds);  

    //add child sockets to set 
    for (int i = 0 ; i < (MAX_CLIENTS + 1); i++)  
    {  
      //socket descriptor 
      sd = sockFDList[i];  

      //if valid socket descriptor then add to read list 
      if(sd > 0)  
        FD_SET(sd, &readfds);  

      //highest file descriptor number, need it for the select function 
      if(sd > max_sd)  
        max_sd = sd;  
    }  

    cout << endl;

    currCmd.CurrGameID  = gameID;
    currCmd.cmd         = GAME_CMD::PLAY;
    currCmd.PlayerID    = PLAYER_ID::PID1;
    currCmd.PlayHistory = (void *)(GameHistory);

    cout << "sending game start for ID --> " << currCmd.CurrGameID << endl;

    if(count == 0)
    {
      //sleep(0.05);
      if((send(sockFDList[1], &currCmd, sizeof(COMMAND), 0)) < sizeof(COMMAND))
      {
        cout << "cmd send failed " << endl;
      }
      
      currCmd.PlayerID   = PLAYER_ID::PID2;

      //sleep(0.05);
      if((send(sockFDList[2], &currCmd, sizeof(COMMAND), 0)) < sizeof(COMMAND))
      {
        cout << "cmd send failed " << endl;
      }
    }

    //wait for an activity on one of the sockets , timeout is NULL ,so wait indefinitely 
    activity = select(max_sd + 1 , &readfds , NULL , NULL , NULL);  
  
    if((activity < 0) && (errno != EINTR))  
    {  
      cout << "select error" << endl;  
    }

    //else its some IO operation on some other socket
    for (int i = 0; i < MAX_CLIENTS; i++)  
    {  
      int sd = client_socket[i];
          
      if (FD_ISSET(sd, &readfds))  
      {  
        if ((valread = recv(sd, &resp, sizeof(PlayerResponse), 0)) > 0)  
        {  
          //cout << "main thread :: received data for sockfd --> " << sd << endl;
          cout << "main thread :: server data received from pID --> " << (int)(resp.pID) << " | played --> " << (int)(resp.ePlay) \
            << " | for gameID --> " << resp.currGameID << endl; 

          if((resp.pID == PLAYER_ID::PID1) && (resp.currGameID == gameID))
          {
            currGameRes.P1Play = resp.ePlay;
            count++;
          }

          if((resp.pID == PLAYER_ID::PID2) && (resp.currGameID == gameID))
          {
            currGameRes.P2Play = resp.ePlay;
            count++;
          }

          if(count == MAX_CLIENTS)
          {
            cout << "received game plays from both players for game ID --> " << resp.currGameID << " | sending next game start ... " << endl;
            count = 0;
            gameCnt++;
            gameID++;

            currGameRes.Round = resp.currGameID;
            currGameRes.Winner = DeclareWinner(currGameRes);

            GameHistory->push_back(currGameRes);

            Json::Value val = CreateJSONObject(currGameRes);
            AddToJSONGameHistory(JSONGameHistory, val);

            cout << "Round --> " << currGameRes.Round << " | p1 played --> " << (int)(currGameRes.P1Play) <<  " | p2 played --> " << (int)(currGameRes.P2Play) \
              << " | Winner is player --> " << (int)(currGameRes.Winner) << endl;

            memset(&currGameRes, 0, sizeof(GameResult));
            break;
          }
          memset(&resp, 0, sizeof(PlayerResponse));
        }
      }  
    }
    if(gameCnt >= MAX_GAME_COUNT)
    {
      cout << "main thread :: GAME END !!!" << endl;
      SerializeJSON(JSONGameHistory);
      break;
    }
  }
  delete GameHistory;
}

int main()
{
  std::vector<int> sockFDList;

  std::thread p1(Player1Thread);
  std::thread p2(Player2Thread);
 
  sockFDList = CreateServer();

  GamePlay(sockFDList);
    
  p1.join();
  p2.join();

  sleep(0.5);
  cout << "closing server and client sockets ... " << endl;
  shutdown(sockFDList[0], SHUT_RDWR);
  shutdown(sockFDList[1], SHUT_RDWR);
  shutdown(sockFDList[2], SHUT_RDWR);
  close(sockFDList[0]);
  close(sockFDList[1]);
  close(sockFDList[2]);

  return 0;
}