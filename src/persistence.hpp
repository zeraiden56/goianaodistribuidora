#pragma once
#include "game.hpp"
#include "player.hpp"
#include "settings.hpp"
#include <filesystem>
#include <iosfwd>

bool validGame(const Game& game,const Player& player);
void encodeGame(std::ostream& stream,const Game& game,const Player& player);
bool decodeGame(std::istream& stream,Game& game,Player& player);
bool saveGame(const std::filesystem::path& file,const Game& game,const Player& player,std::string& error);
bool loadGame(const std::filesystem::path& file,Game& game,Player& player,std::string& error);
bool saveSettings(const std::filesystem::path& file,const Settings& settings,std::string& error);
bool loadSettings(const std::filesystem::path& file,Settings& settings);
