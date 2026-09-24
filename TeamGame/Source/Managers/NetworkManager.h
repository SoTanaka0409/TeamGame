#pragma once
#include "DxLib.h"
#include "PacketTypes.h"
#include <vector>

class NetworkManager
{
private:
    NetworkManager();
    ~NetworkManager();

    int m_netHandle;
    bool m_isConnected;
    bool m_isHost;
    bool m_isListening;

public:
    static NetworkManager& GetInstance()
    {
        static NetworkManager instance;
        return instance;
    }

    bool Connect(const char* ip, int port);
    bool Listen(int port);
    void UpdateConnection();
    void Disconnect();

    bool IsConnected() const { return m_isConnected; }
    bool IsHost() const { return m_isHost; }
    bool IsListening() const { return m_isListening; }

    // 送信
    void SendPacket(const void* data, int size);
    
    // 受信（溜まっているパケットを全て取得）
    std::vector<std::vector<uint8_t>> ReceivePackets();
};
