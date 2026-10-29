// winsock2.h は windows.h より先にインクルードする必要がある（接続診断用の生ソケットで使用）
#include <winsock2.h>
#include <ws2tcpip.h>
#include "NetworkManager.h"
#include <cstdarg>
#include <cstdio>
#include <chrono>

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

    static bool s_firstWrite = true;
    FILE* fp = nullptr;
    if (fopen_s(&fp, "network_log.txt", s_firstWrite ? "w" : "a") == 0 && fp)
    {
        if (s_firstWrite)
        {
            char cwd[MAX_PATH] = { 0 };
            GetCurrentDirectoryA(MAX_PATH, cwd);
            fprintf(fp, "===== TeamGame network log (cwd: %s) =====\n", cwd);
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

// パケット種別から固定長のパケットサイズを返す (未知の種別は0)
size_t NetworkManager::GetPacketSize(uint8_t type)
{
    switch (static_cast<PacketType>(type))
    {
    case PacketType::STAGE_INIT:   return sizeof(PacketStageInit);
    case PacketType::PLAYER_STATE: return sizeof(PacketPlayerState);
    case PacketType::ENEMY_STATE:  return sizeof(PacketEnemyState);
    case PacketType::SHOOT:        return sizeof(PacketShoot);
    }
    return 0;
}

NetworkManager::NetworkManager()
    : m_networkMode(NetworkMode::NONE),
      m_isConnected(false),
      m_isHost(false),
      m_netHandle(-1),
      m_isListening(false)
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

    if (m_networkMode == NetworkMode::DXLIB_LAN)
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
    m_lanRecvBuffer.clear();

    m_isConnected = false;
    m_isHost = false;
    m_isListening = false;
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

    if (m_networkMode == NetworkMode::DXLIB_LAN && m_netHandle != -1)
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

    if (m_networkMode == NetworkMode::DXLIB_LAN && m_isConnected && m_netHandle != -1)
    {
        int lost = GetLostNetWork();
        if (lost != -1)
        {
            Log("[RECV] connection LOST detected: lostHandle=%d (my handle=%d) -> peer closed or network dropped", lost, m_netHandle);
        }

        // TCPはストリームなので、届いたバイト列を溜めてパケット種別ごとのサイズで切り出す
        while (GetNetWorkDataLength(m_netHandle) > 0)
        {
            int dataLength = GetNetWorkDataLength(m_netHandle);
            size_t oldSize = m_lanRecvBuffer.size();
            m_lanRecvBuffer.resize(oldSize + dataLength);
            NetWorkRecv(m_netHandle, m_lanRecvBuffer.data() + oldSize, dataLength);
        }

        size_t offset = 0;
        while (offset < m_lanRecvBuffer.size())
        {
            size_t packetSize = GetPacketSize(m_lanRecvBuffer[offset]);
            if (packetSize == 0)
            {
                // 未知の種別: 以降の区切りが分からないため破棄して同期を取り直す
                Log("[RECV] unknown packet type=%d, discarding %d buffered bytes",
                    static_cast<int>(m_lanRecvBuffer[offset]), static_cast<int>(m_lanRecvBuffer.size() - offset));
                offset = m_lanRecvBuffer.size();
                break;
            }
            if (m_lanRecvBuffer.size() - offset < packetSize) break; // 続きが未着

            packets.emplace_back(m_lanRecvBuffer.begin() + offset, m_lanRecvBuffer.begin() + offset + packetSize);
            offset += packetSize;
        }
        m_lanRecvBuffer.erase(m_lanRecvBuffer.begin(), m_lanRecvBuffer.begin() + offset);
    }

    m_recvCount += static_cast<int>(packets.size());
    if (!packets.empty() && (m_recvCount <= 3 || (m_recvCount % 600) < static_cast<int>(packets.size())))
    {
        Log("[RECV] total=%d (this frame: %d packets, first size=%d)", m_recvCount, static_cast<int>(packets.size()), static_cast<int>(packets[0].size()));
    }
    return packets;
}
