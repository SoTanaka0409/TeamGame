#pragma once
#include"Player.h"
#include"Enemy.h"

class CharacterFactory
{
    CharacterFactory();

    ~CharacterFactory();

    void CreatPlayer(const std::string &type, float StartX, float StartY,int teamId);

    void CreatEnemy(const std::string &type, float StartX, float StartY,int teamId);

    


};
