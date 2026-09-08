#include "NetworkManager.h"

NetworkManager::NetworkManager() : m_netHandle(-1), m_isConnected(false), m_isHost(false), m_isListening(false)
{
}

NetworkManager::~NetworkManager()
{
    Disconnect();
}

bool NetworkManager::Connect(const char* ip, int port)
{
    IPDATA ipdata;
    GetHostIPbyName(ip, &ipdata);
    m_netHandle = ConnectNetWork(ipdata, port);
    if (m_netHandle != -1)
    {
        m_isConnected = true;
        m_isHost = false;
        m_isListening = false;
        return true;
    }
    return false;
}

bool NetworkManager::Listen(int port)
{
    if (PreparationListenNetWork(port) == 0)
    {
        m_isListening = true;
        m_isHost = true;
        return true;
    }
    return false;
}

void NetworkManager::UpdateConnection()
{
    if (m_isListening && !m_isConnected)
    {
        m_netHandle = GetNewAcceptNetWork();
        if (m_netHandle != -1)
        {
            m_isConnected = true;
            m_isListening = false;
            StopListenNetWork();
        }
    }
}

void NetworkManager::Disconnect()
{
    if (m_netHandle != -1)
    {
        CloseNetWork(m_netHandle);
        m_netHandle = -1;
    }
    if (m_isListening)
    {
        StopListenNetWork();
        m_isListening = false;
    }
    m_isConnected = false;
    m_isHost = false;
}

void NetworkManager::SendPacket(const void* data, int size)
{
    if (m_isConnected && m_netHandle != -1)
    {
        NetWorkSend(m_netHandle, const_cast<void*>(data), size);
    }
}

std::vector<std::vector<uint8_t>> NetworkManager::ReceivePackets()
{
    std::vector<std::vector<uint8_t>> packets;
    if (m_isConnected && m_netHandle != -1)
    {
        while (GetNetWorkDataLength(m_netHandle) > 0)
        {
            int dataLength = GetNetWorkDataLength(m_netHandle);
            std::vector<uint8_t> buffer(dataLength);
            NetWorkRecv(m_netHandle, buffer.data(), dataLength);
            
            // DXLibのNetWorkRecvはTCPストリームを結合/分割する可能性があるため
            // 本格的な実装ではサイズヘッダ等で分割・結合処理が必要ですが、
            // 今回は簡易的にそのままパケットとして扱います
            packets.push_back(buffer);
        }
    }
    return packets;
}
