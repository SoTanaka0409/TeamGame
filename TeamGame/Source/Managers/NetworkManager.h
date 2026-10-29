#pragma once
#include "DxLib.h"
#include "PacketTypes.h"
#include <windows.h>
#include <string>
#include <vector>
#include <mutex>

/**
 * @brief ネットワーク通信モード
 */
enum class NetworkMode
{
    NONE,       ///< 未接続
    DXLIB_LAN   ///< LAN直接通信（DxLib）
};

/**
 * @brief ネットワーク通信を管理するシングルトンクラス
 * @details クライアント・ホスト間の接続、パケットの送受信、切断処理などを管理する。
 *          ローカルLAN（同一ネットワーク）での直接対戦に対応。
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

    // LAN(TCP)受信用: パケット境界をまたいで届いたバイト列を溜めておく
    std::vector<uint8_t> m_lanRecvBuffer;
    static size_t GetPacketSize(uint8_t type);

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

    // LAN通信
    bool Connect(const char* ip, int port);
    bool Listen(int port);

    void UpdateConnection();
    void Disconnect();

    bool IsConnected() const { return m_isConnected; }
    bool IsHost() const { return m_isHost; }
    bool IsListening() const { return m_isListening; }
    NetworkMode GetMode() const { return m_networkMode; }

    void SendPacket(const void* data, int size);
    std::vector<std::vector<uint8_t>> ReceivePackets();
};
