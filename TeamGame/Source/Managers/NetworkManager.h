#pragma once
#include "DxLib.h"
#include "PacketTypes.h"
#include <vector>

/**
 * @brief ネットワーク通信を管理するシングルトンクラス
 * @details クライアント・ホスト間の接続、パケットの送受信、切断処理などを管理する。
 */
class NetworkManager
{
private:
    /**
     * @brief コンストラクタ
     */
    NetworkManager();

    /**
     * @brief デストラクタ
     */
    ~NetworkManager();

    int m_netHandle;
    bool m_isConnected;
    bool m_isHost;
    bool m_isListening;

public:
    /**
     * @brief インスタンスの取得
     * @return NetworkManager& ネットワークマネージャのシングルトンインスタンス
     */
    static NetworkManager& GetInstance()
    {
        static NetworkManager instance;
        return instance;
    }

    /**
     * @brief サーバーへの接続
     * @param ip 接続先のIPアドレス
     * @param port 接続先のポート番号
     * @return bool 接続に成功した場合はtrue、失敗した場合はfalse
     * @details 指定したIPおよびポートに対してクライアントとして接続を試みる。
     */
    bool Connect(const char* ip, int port);

    /**
     * @brief 接続待ち（ホスト側）
     * @param port 待ち受けポート番号
     * @return bool 待ち受け開始に成功した場合はtrue、失敗した場合はfalse
     * @details 指定したポートで他のクライアントからの接続を待機する。
     */
    bool Listen(int port);

    /**
     * @brief 通信状態の更新
     * @details 新規接続の受付など、毎フレーム行う必要のあるネットワーク状態の更新処理を行う。
     */
    void UpdateConnection();

    /**
     * @brief 接続の切断
     * @details 現在のネットワーク接続を切断し、初期状態に戻す。
     */
    void Disconnect();

    /**
     * @brief 接続状態の取得
     * @return bool 接続中の場合はtrue
     */
    bool IsConnected() const { return m_isConnected; }

    /**
     * @brief ホスト判定
     * @return bool 自身がホストである場合はtrue
     */
    bool IsHost() const { return m_isHost; }

    /**
     * @brief 接続待ち状態の取得
     * @return bool 接続待ち中の場合はtrue
     */
    bool IsListening() const { return m_isListening; }

    /**
     * @brief パケットの送信
     * @param data 送信するデータのポインタ
     * @param size 送信するデータのサイズ
     * @details 接続先の相手に対してデータを送信する。
     */
    void SendPacket(const void* data, int size);
    
    /**
     * @brief パケットの受信
     * @return std::vector<std::vector<uint8_t>> 受信したパケットのリスト
     * @details 溜まっている受信データを全て取得し、パケット単位のリストとして返す。
     */
    std::vector<std::vector<uint8_t>> ReceivePackets();
};
