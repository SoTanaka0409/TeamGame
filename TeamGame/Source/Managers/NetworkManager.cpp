#include "NetworkManager.h"

#pragma comment(lib, "winhttp.lib")

NetworkManager::NetworkManager()
    : m_networkMode(NetworkMode::NONE),
      m_isConnected(false),
      m_isHost(false),
      m_netHandle(-1),
      m_isListening(false),
      m_roomCode(""),
      m_hasGuestJoined(false),
      m_hSession(NULL),
      m_hConnect(NULL),
      m_hWebSocket(NULL),
      m_wsRunning(false)
{
}

NetworkManager::~NetworkManager()
{
    Disconnect();
}

bool NetworkManager::Connect(const char* ip, int port)
{
    Disconnect();
    IPDATA ipdata;
    GetHostIPbyName(ip, &ipdata);
    m_netHandle = ConnectNetWork(ipdata, port);
    if (m_netHandle != -1)
    {
        m_isConnected = true;
        m_isHost = false;
        m_isListening = false;
        m_networkMode = NetworkMode::DXLIB_LAN;
        return true;
    }
    return false;
}

bool NetworkManager::Listen(int port)
{
    Disconnect();
    if (PreparationListenNetWork(port) == 0)
    {
        m_isListening = true;
        m_isHost = true;
        m_networkMode = NetworkMode::DXLIB_LAN;
        return true;
    }
    return false;
}

bool NetworkManager::ConnectWeb(const std::string& host, int port, const std::string& roomCode, bool isHost, bool useSSL)
{
    Disconnect();

    m_roomCode = roomCode;
    m_isHost = isHost;
    m_hasGuestJoined = false;

    // 1. セッション作成
    m_hSession = WinHttpOpen(L"TeamGameClient/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!m_hSession) return false;

    // タイムアウト設定 (接続: 5秒, 送信: 5秒, 受信: 5秒)
    WinHttpSetTimeouts(m_hSession, 5000, 5000, 5000, 5000);

    // 2. サーバーへの接続
    std::wstring wHost(host.begin(), host.end());
    m_hConnect = WinHttpConnect(m_hSession, wHost.c_str(), static_cast<INTERNET_PORT>(port), 0);
    if (!m_hConnect)
    {
        Disconnect();
        return false;
    }

    // 3. リクエストパス構築
    std::wstring wRoom(roomCode.begin(), roomCode.end());
    std::wstring wRole = isHost ? L"host" : L"guest";
    std::wstring path = L"/ws?room=" + wRoom + L"&role=" + wRole;

    DWORD openFlags = useSSL ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(m_hConnect, L"GET", path.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, openFlags);
    if (!hRequest)
    {
        Disconnect();
        return false;
    }

    if (useSSL)
    {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }

    // 4. WebSocketアップグレードオプション
    if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0))
    {
        WinHttpCloseHandle(hRequest);
        Disconnect();
        return false;
    }

    // 5. リクエスト送信
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0))
    {
        WinHttpCloseHandle(hRequest);
        Disconnect();
        return false;
    }

    // 6. レスポンス受信
    if (!WinHttpReceiveResponse(hRequest, NULL))
    {
        WinHttpCloseHandle(hRequest);
        Disconnect();
        return false;
    }

    // ステータスコードチェック (101 Switching Protocols)
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) ||
        statusCode != 101)
    {
        WinHttpCloseHandle(hRequest);
        Disconnect();
        return false;
    }

    // 7. WebSocketアップグレード完了
    m_hWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest, NULL);
    WinHttpCloseHandle(hRequest);
    if (!m_hWebSocket)
    {
        Disconnect();
        return false;
    }

    // 8. 受信スレッド起動
    m_wsRunning = true;
    m_wsThread = std::thread(&NetworkManager::WebSocketReceiveLoop, this);

    m_isConnected = true;
    m_networkMode = NetworkMode::WEBSOCKET;
    return true;
}

void NetworkManager::WebSocketReceiveLoop()
{
    std::vector<uint8_t> buffer(8192);
    while (m_wsRunning && m_hWebSocket != NULL)
    {
        DWORD bytesRead = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;
        DWORD err = WinHttpWebSocketReceive(m_hWebSocket, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, &bufferType);

        if (err == ERROR_SUCCESS && bytesRead > 0)
        {
            if (bufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE)
            {
                m_hasGuestJoined = true;
                std::vector<uint8_t> packet(buffer.begin(), buffer.begin() + bytesRead);
                {
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    m_recvQueue.push(std::move(packet));
                }
            }
            else if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)
            {
                std::string msg(reinterpret_cast<char*>(buffer.data()), bytesRead);
                if (msg == "GUEST_JOINED")
                {
                    m_hasGuestJoined = true;
                }
            }
            else if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
            {
                break;
            }
        }
        else if (err != ERROR_SUCCESS)
        {
            break;
        }
    }
    m_isConnected = false;
}

void NetworkManager::UpdateConnection()
{
    if (m_networkMode == NetworkMode::DXLIB_LAN && m_isListening && !m_isConnected)
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
    if (m_networkMode == NetworkMode::WEBSOCKET)
    {
        m_wsRunning = false;
        if (m_hWebSocket != NULL)
        {
            WinHttpWebSocketClose(m_hWebSocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
            WinHttpCloseHandle(m_hWebSocket);
            m_hWebSocket = NULL;
        }
        if (m_hConnect != NULL)
        {
            WinHttpCloseHandle(m_hConnect);
            m_hConnect = NULL;
        }
        if (m_hSession != NULL)
        {
            WinHttpCloseHandle(m_hSession);
            m_hSession = NULL;
        }
        if (m_wsThread.joinable())
        {
            m_wsThread.join();
        }
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_recvQueue.empty()) m_recvQueue.pop();
    }
    else if (m_networkMode == NetworkMode::DXLIB_LAN)
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
    }

    m_isConnected = false;
    m_isHost = false;
    m_isListening = false;
    m_hasGuestJoined = false;
    m_networkMode = NetworkMode::NONE;
}

void NetworkManager::SendPacket(const void* data, int size)
{
    if (!m_isConnected) return;

    if (m_networkMode == NetworkMode::WEBSOCKET && m_hWebSocket != NULL)
    {
        WinHttpWebSocketSend(m_hWebSocket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, const_cast<void*>(data), size);
    }
    else if (m_networkMode == NetworkMode::DXLIB_LAN && m_netHandle != -1)
    {
        NetWorkSend(m_netHandle, const_cast<void*>(data), size);
    }
}

std::vector<std::vector<uint8_t>> NetworkManager::ReceivePackets()
{
    std::vector<std::vector<uint8_t>> packets;

    if (m_networkMode == NetworkMode::WEBSOCKET)
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_recvQueue.empty())
        {
            packets.push_back(std::move(m_recvQueue.front()));
            m_recvQueue.pop();
        }
    }
    else if (m_networkMode == NetworkMode::DXLIB_LAN && m_isConnected && m_netHandle != -1)
    {
        while (GetNetWorkDataLength(m_netHandle) > 0)
        {
            int dataLength = GetNetWorkDataLength(m_netHandle);
            std::vector<uint8_t> buffer(dataLength);
            NetWorkRecv(m_netHandle, buffer.data(), dataLength);
            packets.push_back(buffer);
        }
    }
    return packets;
}
