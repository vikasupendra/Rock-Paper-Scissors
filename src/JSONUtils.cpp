#include "JSONUtils.h"

using namespace std;

std::string GetJSONPlayAttributeString(const MYPLAY  &play);
std::string GetJSONMappingWinnerString(const GameResult &currGameResult, JSONAttribute attrib);

std::string GetJSONPlayAttributeString(const MYPLAY  &play)
{
  switch(play)
  {
  case MYPLAY::ROCK:
    {
      return ROCK_STRING;
    }
  case MYPLAY::PAPER:
    {
      return PAPER_STRING;
    }
  case MYPLAY::SCISSORS:
    {
      return SCISSORS_STRING;
    }
  default:
    return "";
  }
}

void AddToJSONGameHistory(std::vector<Json::Value> &gameHistory, const Json::Value &gameInfoObj)
{
  gameHistory.push_back(gameInfoObj);
}

std::string GetJSONMappingWinnerString(const GameResult &currGameResult, JSONAttribute attrib)
{
  if(attrib == JSONAttribute::ATTR_WINNER) 
  {
    if(currGameResult.Winner == PLAYER_ID::PID1)
      return PLAYER1_STRING;
    else if(currGameResult.Winner == PLAYER_ID::PID2)
      return PLAYER2_STRING;
    else if(currGameResult.Winner == PLAYER_ID::PID_NULL)
      return NULL_STRING;
  }
}

Json::Value CreateJSONObject(const GameResult &currGameResult)
{
  Json::Value nested;
  Json::Value root;
  nested["Player1"] = GetJSONPlayAttributeString (currGameResult.P1Play);
  nested["Player2"] = GetJSONPlayAttributeString (currGameResult.P2Play);

  std::string winner = GetJSONMappingWinnerString (currGameResult, JSONAttribute::ATTR_WINNER);
  if(winner == NULL_STRING)
    root["Winner"] = Json::nullValue;
  else
    root["Winner"] = winner;


  root["Round"] = currGameResult.Round;
  root["Inputs"] = nested;    

  return root;
}

void SerializeJSON(const std::vector<Json::Value> &GameHistory)
{
  std::ofstream outFile;

  // Write the output to a file
  outFile.open("result.json");

  for(auto it : GameHistory)
  {
    outFile << it.toStyledString();
  }
  outFile.close(); 
}