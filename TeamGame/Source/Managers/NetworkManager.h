#pragma once
#include "DxLib.h"
#include "PacketTypes.h"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>

/**
 * @brief ネットワーク通信モード
 */
enum class NetworkMode
{
    NONE,       ///< 未接続
    DXLIB_LAN,  ///< 従来のLAN直接通信（DxLib）
    WEBSOCKET   ///< Web（WebSocket）中継通信
};

/**
 * @brief ネットワーク通信を管理するシングルトンクラス
 * @details クライアント・ホスト間の接続、パケットの送受信、切断処理などを管理する。
 *          LAN直接通信（DxLib）およびWeb中継通信（WinHTTP WebSocket）の両方に対応。
 */
class NetworkManager
{
private:
    NetworkManager();
    ~NetworkManager();

    // 共通状態
    NetworkMode m_networkMode;
    bool m_isConnected;
    bool m_isHost;

    // DxLib LAN通信用
    int m_netHandle;
    bool m_isListening;

    // WebSocket通信用
    std::string m_roomCode;
    std::atomic<bool> m_hasGuestJoined;
    HINTERNET m_hSession;
    HINTERNET m_hConnect;
    HINTERNET m_hWebSocket;
    std::thread m_wsThread;
    std::atomic<bool> m_wsRunning;
    std::mutex m_queueMutex;
    std::queue<std::vector<uint8_t>> m_recvQueue;

    // LAN(TCP)受信用: パケット境界をまたいで届いたバイト列を溜めておく
    std::vector<uint8_t> m_lanRecvBuffer;
    static size_t GetPacketSize(uint8_t type);

    void WebSocketReceiveLoop();

    // 通信診断用カウンタ
    int m_logTick = 0;
    int m_sendCount = 0;
    int m_recvCount = 0;
    int m_sendFailCount = 0;

public:
    /**
     * @brief 通信ログを network_log.txt (実行ファイルと同じフォルダ) に追記する (スレッドセーフ)
     */
    static void Log(const char* fmt, ...);

    static NetworkManager& GetInstance()
    {
        static NetworkManager instance;
        return instance;
    }

    // LAN通信（既存）
    bool Connect(const char* ip, int port);
    bool Listen(int port);

    // Web（WebSocket）通信
    bool ConnectWeb(const std::string& host, int port, const std::string& roomCode, bool isHost, bool useSSL);

    void UpdateConnection();
    void Disconnect();

    bool IsConnected() const { return m_isConnected; }
    bool IsHost() const { return m_isHost; }
    bool IsListening() const { return m_isListening; }
    NetworkMode GetMode() const { return m_networkMode; }
    std::string GetRoomCode() const { return m_roomCode; }
    bool HasGuestJoined() const { return m_hasGuestJoined; }

    void SendPacket(const void* data, int size);
    std::vector<std::vector<uint8_t>> ReceivePackets();
};
