#pragma once

class GameSettings
{
private:
    GameSettings() : isAimLockHoldMode(true) {}
    ~GameSettings() = default;

public:
    static GameSettings& GetInstance()
    {
        static GameSettings instance;
        return instance;
    }

    bool isAimLockHoldMode;
};
