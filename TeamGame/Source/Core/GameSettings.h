#pragma once

class GameSettings
{
private:
    GameSettings() : isAimLockHoldMode(true), isBloodSplatterEnabled(true) {}
    ~GameSettings() = default;

public:
    static GameSettings& GetInstance()
    {
        static GameSettings instance;
        return instance;
    }

    bool isAimLockHoldMode;
    bool isBloodSplatterEnabled;
};
