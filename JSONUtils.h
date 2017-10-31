#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include "players.h"
#include "json/json.h"

enum class  JSONAttribute {ATTR_INVALID, ATTR_ROUND, ATTR_WINNER, ATTR_PLAY};

#define PLAYER1_STRING  (std::string)"Player1"
#define PLAYER2_STRING  (std::string)"Player2"
#define ROCK_STRING     (std::string)"Rock"
#define PAPER_STRING    (std::string)"Paper"
#define SCISSORS_STRING (std::string)"Scissors"
#define NULL_STRING     (std::string)"null"

Json::Value CreateJSONObject(const GameResult &currGameResult);
void SerializeJSON(const std::vector<Json::Value> &GameHistory);
void AddToJSONGameHistory(std::vector<Json::Value> &gameHistory, const Json::Value &gameInfoObj);