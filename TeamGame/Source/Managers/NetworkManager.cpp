// winsock2.h は windows.h より先にインクルードする必要がある（接続診断用の生ソケットで使用）
#include <winsock2.h>
#include <ws2tcpip.h>
#include "NetworkManager.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <chrono>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32.lib")

// ============================================================================
// 通信ログ (network_log.txt)
// ============================================================================
void NetworkManager::Log(const char* fmt, ...)
{
    static std::mutex s_logMutex;
    std::lock_guard<std::mutex> lock(s_logMutex);

    char body[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);

    SYSTEMTIME st;
    GetLocalTime(&st);
    char line[1200];
    snprintf(line, sizeof(line), "[%02d:%02d:%02d.%03d] %s\n",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, body);

    // ログは実行ファイル(.exe)と同じフォルダに出力する (カレントディレクトリの変更に左右されない)
    static char s_logPath[MAX_PATH] = { 0 };
    if (s_logPath[0] == '\0')
    {
        DWORD len = GetModuleFileNameA(NULL, s_logPath, MAX_PATH);
        char* lastSep = (len > 0) ? strrchr(s_logPath, '\\') : nullptr;
        if (lastSep) strcpy_s(lastSep + 1, MAX_PATH - (lastSep + 1 - s_logPath), "network_log.txt");
        else strcpy_s(s_logPath, "network_log.txt");
    }

    static bool s_firstWrite = true;
    FILE* fp = nullptr;
    if (fopen_s(&fp, s_logPath, s_firstWrite ? "w" : "a") == 0 && fp)
    {
        if (s_firstWrite)
        {
            char cwd[MAX_PATH] = { 0 };
            GetCurrentDirectoryA(MAX_PATH, cwd);
            fprintf(fp, "===== TeamGame network log (file: %s, cwd: %s) =====\n", s_logPath, cwd);
            s_firstWrite = false;
        }
        fputs(line, fp);
        fclose(fp);
    }
    OutputDebugStringA(line);
}

// 自PCのIPアドレス一覧をログ出力（複数アダプタ・VPN・仮想NICの混在確認用）
static void LogLocalIPs()
{
    IPDATA ips[16];
    int num = 0;
    if (GetMyIPAddress(ips, 16, &num) == 0 && num > 0)
    {
        for (int i = 0; i < num; i++)
        {
            NetworkManager::Log("  local IP[%d] = %d.%d.%d.%d", i, ips[i].d1, ips[i].d2, ips[i].d3, ips[i].d4);
        }
    }
    else
    {
        NetworkManager::Log("  GetMyIPAddress failed or returned 0 addresses");
    }
}

// 生のTCPソケットで接続を試行し、失敗理由(WSAエラーコード)を特定する
// 10060(WSAETIMEDOUT)=応答なし(ファイアウォールでDROPされている可能性大) / 10061(WSAECONNREFUSED)=到達したが誰もListenしていない
static int ProbeTcp(const char* ip, int port, int timeoutMs)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;

    int result = -1;
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%d", port);

    if (getaddrinfo(ip, portStr, &hints, &res) != 0 || !res)
    {
        result = WSAGetLastError();
        NetworkManager::Log("  [probe] getaddrinfo('%s') failed, WSA error=%d", ip, result);
        WSACleanup();
        return result;
    }

    SOCKET s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s == INVALID_SOCKET)
    {
        result = WSAGetLastError();
    }
    else
    {
        u_long nonBlock = 1;
        ioctlsocket(s, FIONBIO, &nonBlock);
        int c = connect(s, res->ai_addr, static_cast<int>(res->ai_addrlen));
        if (c == 0)
        {
            result = 0;
        }
        else if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            fd_set wfds, efds;
            FD_ZERO(&wfds); FD_ZERO(&efds);
            FD_SET(s, &wfds); FD_SET(s, &efds);
            timeval tv;
            tv.tv_sec = timeoutMs / 1000;
            tv.tv_usec = (timeoutMs % 1000) * 1000;
            int sel = select(0, nullptr, &wfds, &efds, &tv);
            if (sel == 0)
            {
                result = WSAETIMEDOUT;
            }
            else
            {
                int soErr = 0;
                int len = sizeof(soErr);
                getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soErr), &len);
                result = soErr;
            }
        }
        else
        {
            result = WSAGetLastError();
        }
        closesocket(s);
    }
    freeaddrinfo(res);
    WSACleanup();

    const char* meaning =
        (result == 0) ? "CONNECT OK (TCP reachable. Problem is above TCP, e.g. protocol/port mismatch)" :
        (result == WSAETIMEDOUT) ? "TIMEOUT (no reply: firewall DROP / wrong IP / host not on same network)" :
        (result == WSAECONNREFUSED) ? "REFUSED (host reachable but nobody is listening on this port)" :
        (result == WSAEHOSTUNREACH) ? "HOST UNREACHABLE (routing problem)" :
        (result == WSAENETUNREACH) ? "NETWORK UNREACHABLE (routing/subnet problem)" : "other error";
    NetworkManager::Log("  [probe] raw TCP connect to %s:%d -> WSA error=%d : %s", ip, port, result, meaning);
    return result;
}

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
    Log("[LAN] Connect() start: target='%s' port=%d", ip, port);
    LogLocalIPs();

    IPDATA ipdata = {};
    int resolveRet = GetHostIPbyName(ip, &ipdata);
    Log("[LAN] GetHostIPbyName('%s') ret=%d -> %d.%d.%d.%d", ip, resolveRet, ipdata.d1, ipdata.d2, ipdata.d3, ipdata.d4);
    if (resolveRet != 0)
    {
        Log("[LAN] WARNING: could not resolve address. Check for typos / trailing spaces in the IP.");
    }

    auto t0 = std::chrono::steady_clock::now();
    m_netHandle = ConnectNetWork(ipdata, port);
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    Log("[LAN] ConnectNetWork() returned handle=%d (took %lld ms)", m_netHandle, ms);

    if (m_netHandle != -1)
    {
        m_isConnected = true;
        m_isHost = false;
        m_isListening = false;
        m_networkMode = NetworkMode::DXLIB_LAN;
        m_sendCount = m_recvCount = m_sendFailCount = 0;
        Log("[LAN] Connected as CLIENT. GetNetWorkAcceptState=%d", GetNetWorkAcceptState(m_netHandle));
        return true;
    }

    Log("[LAN] Connect FAILED. Running raw TCP probe to identify the reason...");
    ProbeTcp(ip, port, 3000);
    return false;
}

bool NetworkManager::Listen(int port)
{
    Disconnect();
    Log("[LAN] Listen() start: port=%d", port);
    LogLocalIPs();

    int ret = PreparationListenNetWork(port);
    Log("[LAN] PreparationListenNetWork(%d) ret=%d (0=OK)", port, ret);
    if (ret == 0)
    {
        m_isListening = true;
        m_isHost = true;
        m_networkMode = NetworkMode::DXLIB_LAN;
        m_logTick = 0;
        m_sendCount = m_recvCount = m_sendFailCount = 0;
        Log("[LAN] Now listening on TCP %d. If the client times out, check Windows Firewall inbound rule for this port/exe.", port);
        return true;
    }
    Log("[LAN] Listen FAILED. Port may already be in use (another TeamGame instance?) or blocked.");
    return false;
}

bool NetworkManager::ConnectWeb(const std::string& host, int port, const std::string& roomCode, bool isHost, bool useSSL)
{
    Disconnect();
    Log("[WEB] ConnectWeb() start: host='%s' port=%d room='%s' role=%s ssl=%d",
        host.c_str(), port, roomCode.c_str(), isHost ? "host" : "guest", useSSL ? 1 : 0);

    m_roomCode = roomCode;
    m_isHost = isHost;
    m_hasGuestJoined = false;

    // 1. セッション作成
    m_hSession = WinHttpOpen(L"TeamGameClient/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!m_hSession)
    {
        Log("[WEB] WinHttpOpen failed, GetLastError=%lu", GetLastError());
        return false;
    }

    // タイムアウト設定 (接続: 5秒, 送信: 5秒, 受信: 5秒)
    WinHttpSetTimeouts(m_hSession, 5000, 5000, 5000, 5000);

    // 2. サーバーへの接続
    std::wstring wHost(host.begin(), host.end());
    m_hConnect = WinHttpConnect(m_hSession, wHost.c_str(), static_cast<INTERNET_PORT>(port), 0);
    if (!m_hConnect)
    {
        Log("[WEB] WinHttpConnect failed, GetLastError=%lu (12007=DNS resolve failed)", GetLastError());
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
        Log("[WEB] WinHttpOpenRequest failed, GetLastError=%lu", GetLastError());
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
        Log("[WEB] WinHttpSetOption(UPGRADE_TO_WEB_SOCKET) failed, GetLastError=%lu", GetLastError());
        WinHttpCloseHandle(hRequest);
        Disconnect();
        return false;
    }

    // 5. リクエスト送信
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0))
    {
        Log("[WEB] WinHttpSendRequest failed, GetLastError=%lu (12002=timeout, 12029=cannot connect, 12175=SSL error)", GetLastError());
        WinHttpCloseHandle(hRequest);
        Disconnect();
        return false;
    }

    // 6. レスポンス受信
    if (!WinHttpReceiveResponse(hRequest, NULL))
    {
        Log("[WEB] WinHttpReceiveResponse failed, GetLastError=%lu (12002=timeout)", GetLastError());
        WinHttpCloseHandle(hRequest);
        Disconnect();
        return false;
    }

    // ステータスコードチェック (101 Switching Protocols)
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    BOOL qOk = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    Log("[WEB] HTTP status code = %lu (expected 101)", statusCode);
    if (!qOk || statusCode != 101)
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
        Log("[WEB] WinHttpWebSocketCompleteUpgrade failed, GetLastError=%lu", GetLastError());
        Disconnect();
        return false;
    }

    // 8. 受信スレッド起動
    m_wsRunning = true;
    m_wsThread = std::thread(&NetworkManager::WebSocketReceiveLoop, this);

    m_isConnected = true;
    m_networkMode = NetworkMode::WEBSOCKET;
    m_sendCount = m_recvCount = m_sendFailCount = 0;
    Log("[WEB] WebSocket connected.");
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
                Log("[WEB] text message from server: '%s'", msg.c_str());
                if (msg == "GUEST_JOINED")
                {
                    m_hasGuestJoined = true;
                }
            }
            else if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
            {
                Log("[WEB] server sent CLOSE frame");
                break;
            }
        }
        else if (err != ERROR_SUCCESS)
        {
            Log("[WEB] WinHttpWebSocketReceive error=%lu -> receive loop ends", err);
            break;
        }
    }
    m_isConnected = false;
    Log("[WEB] receive loop finished (isConnected=false)");
}

void NetworkManager::UpdateConnection()
{
    if (m_networkMode == NetworkMode::DXLIB_LAN && m_isListening && !m_isConnected)
    {
        // 約5秒ごとに待機状態をログ出力
        if ((m_logTick++ % 300) == 0)
        {
            Log("[LAN] host waiting for client... (listening on TCP, tick=%d)", m_logTick);
        }

        m_netHandle = GetNewAcceptNetWork();
        if (m_netHandle != -1)
        {
            m_isConnected = true;
            m_isListening = false;
            StopListenNetWork();

            IPDATA remote = {};
            GetNetWorkIP(m_netHandle, &remote);
            Log("[LAN] client ACCEPTED: handle=%d remote=%d.%d.%d.%d", m_netHandle, remote.d1, remote.d2, remote.d3, remote.d4);
        }
    }
}

void NetworkManager::Disconnect()
{
    if (m_networkMode != NetworkMode::NONE || m_isConnected || m_isListening)
    {
        Log("Disconnect(): mode=%d connected=%d listening=%d sent=%d recv=%d sendFail=%d",
            static_cast<int>(m_networkMode), m_isConnected ? 1 : 0, m_isListening ? 1 : 0,
            m_sendCount, m_recvCount, m_sendFailCount);
    }

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

    m_sendCount++;
    if (m_sendCount <= 3 || (m_sendCount % 600) == 0)
    {
        Log("[SEND] #%d size=%d type=%d", m_sendCount, size, size > 0 ? static_cast<int>(*reinterpret_cast<const uint8_t*>(data)) : -1);
    }

    if (m_networkMode == NetworkMode::WEBSOCKET && m_hWebSocket != NULL)
    {
        DWORD err = WinHttpWebSocketSend(m_hWebSocket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, const_cast<void*>(data), size);
        if (err != ERROR_SUCCESS)
        {
            m_sendFailCount++;
            if (m_sendFailCount <= 5 || (m_sendFailCount % 120) == 0)
                Log("[SEND] WebSocket send FAILED err=%lu (fail count=%d)", err, m_sendFailCount);
        }
    }
    else if (m_networkMode == NetworkMode::DXLIB_LAN && m_netHandle != -1)
    {
        int ret = NetWorkSend(m_netHandle, const_cast<void*>(data), size);
        if (ret != 0)
        {
            m_sendFailCount++;
            if (m_sendFailCount <= 5 || (m_sendFailCount % 120) == 0)
                Log("[SEND] NetWorkSend FAILED ret=%d (fail count=%d, lostHandle=%d)", ret, m_sendFailCount, GetLostNetWork());
        }
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
        // 相手側の切断を検出してログに残す（動作は従来どおり）
        int lost = GetLostNetWork();
        if (lost != -1)
        {
            Log("[RECV] connection LOST detected: lostHandle=%d (my handle=%d) -> peer closed or network dropped", lost, m_netHandle);
        }

        while (GetNetWorkDataLength(m_netHandle) > 0)
        {
            int dataLength = GetNetWorkDataLength(m_netHandle);
            std::vector<uint8_t> buffer(dataLength);
            NetWorkRecv(m_netHandle, buffer.data(), dataLength);
            packets.push_back(buffer);
        }
    }

    m_recvCount += static_cast<int>(packets.size());
    if (!packets.empty() && (m_recvCount <= 3 || (m_recvCount % 600) < static_cast<int>(packets.size())))
    {
        Log("[RECV] total=%d (this frame: %d packets, first size=%d)", m_recvCount, static_cast<int>(packets.size()), static_cast<int>(packets[0].size()));
    }
    return packets;
}
