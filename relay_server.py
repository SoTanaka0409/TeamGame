import asyncio
import http
import os
import sys
import urllib.parse
from websockets.legacy.server import serve

PORT = int(os.environ.get("PORT", 8080))

# 部屋管理: { room_id: { "host": ws, "guests": set() } }
rooms = {}

HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TeamGame - オンライン対戦サーバー</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #0f172a 0%, #1e1b4b 100%);
            color: #f8fafc;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 40px 20px;
        }
        .container {
            max-width: 800px;
            width: 100%;
            background: rgba(30, 41, 59, 0.8);
            border: 1px solid rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border-radius: 16px;
            padding: 32px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.5);
        }
        h1 {
            font-size: 2.2rem;
            color: #38bdf8;
            margin-bottom: 8px;
            display: flex;
            align-items: center;
            gap: 12px;
        }
        .subtitle {
            color: #94a3b8;
            margin-bottom: 24px;
            font-size: 1.05rem;
        }
        .status-badge {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            background: rgba(34, 197, 94, 0.2);
            color: #4ade80;
            border: 1px solid rgba(34, 197, 94, 0.4);
            padding: 6px 14px;
            border-radius: 9999px;
            font-weight: bold;
            font-size: 0.95rem;
            margin-bottom: 24px;
        }
        .status-dot {
            width: 10px;
            height: 10px;
            background: #22c55e;
            border-radius: 50%;
            box-shadow: 0 0 10px #22c55e;
            animation: pulse 2s infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; transform: scale(1); }
            50% { opacity: 0.5; transform: scale(0.9); }
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 32px;
        }
        .stat-card {
            background: rgba(15, 23, 42, 0.6);
            border: 1px solid rgba(255, 255, 255, 0.05);
            border-radius: 12px;
            padding: 16px;
            text-align: center;
        }
        .stat-val {
            font-size: 2rem;
            font-weight: bold;
            color: #facc15;
        }
        .stat-label {
            color: #94a3b8;
            font-size: 0.85rem;
            margin-top: 4px;
        }
        .section-title {
            font-size: 1.25rem;
            color: #f1f5f9;
            margin: 24px 0 12px 0;
            border-left: 4px solid #38bdf8;
            padding-left: 10px;
        }
        .steps {
            background: rgba(15, 23, 42, 0.5);
            border-radius: 12px;
            padding: 20px;
            list-style-position: inside;
            line-height: 1.8;
            color: #cbd5e1;
        }
        .steps li strong {
            color: #facc15;
        }
        .code-block {
            background: #020617;
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            padding: 12px 16px;
            font-family: Consolas, monospace;
            color: #a5f3fc;
            margin: 12px 0;
            overflow-x: auto;
            font-size: 0.95rem;
        }
        .btn-group {
            display: flex;
            gap: 12px;
            margin-top: 28px;
            flex-wrap: wrap;
        }
        .btn {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            padding: 12px 24px;
            border-radius: 8px;
            text-decoration: none;
            font-weight: bold;
            transition: all 0.2s;
        }
        .btn-primary {
            background: #2563eb;
            color: white;
        }
        .btn-primary:hover {
            background: #1d4ed8;
            transform: translateY(-2px);
        }
        .btn-github {
            background: #334155;
            color: white;
        }
        .btn-github:hover {
            background: #475569;
            transform: translateY(-2px);
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="status-badge">
            <span class="status-dot"></span>
            中継サーバー 正常稼働中 (ONLINE)
        </div>
        <h1>🎮 TeamGame オンライン対戦サーバー</h1>
        <p class="subtitle">2人対戦・協力2Dシューティングアクション Web中継システム</p>

        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-val">{ROOM_COUNT}</div>
                <div class="stat-label">現在の対戦部屋数</div>
            </div>
            <div class="stat-card">
                <div class="stat-val">{CLIENT_COUNT}</div>
                <div class="stat-label">接続中のプレイヤー</div>
            </div>
        </div>

        <h2 class="section-title">🚀 オンライン対戦の遊び方</h2>
        <ol class="steps">
            <li>ゲーム（<code>TeamGame.exe</code>）を起動し、<strong>Web Online (インターネット対戦)</strong> を選択します。</li>
            <li><strong>ホスト側（1人目）</strong>: <strong>Create Room (部屋を作成)</strong> を選ぶと、画面に4桁の部屋番号（例: <code>3842</code>）が表示されます。</li>
            <li><strong>ゲスト側（2人目）</strong>: <strong>Join Room (部屋番号で参加)</strong> を選び、その4桁番号を入力するだけで即座に対戦開始！</li>
        </ol>

        <h2 class="section-title">⚙️ 接続先設定（server_config.ini）</h2>
        <p style="color: #94a3b8; font-size: 0.9rem;">ゲームフォルダ内の <code>server_config.ini</code> にこのサーバーのアドレスを設定してください：</p>
        <div class="code-block">[Server]<br>Host={SERVER_HOST}<br>Port={SERVER_PORT}<br>UseSSL={SERVER_SSL}</div>

        <div class="btn-group">
            <a href="https://github.com/SoTanaka0409/TeamGame" target="_blank" class="btn btn-github">
                🐙 GitHub リポジトリを見る
            </a>
        </div>
    </div>
</body>
</html>
"""

async def process_request(path, request_headers):
    # 普通のブラウザ（HTTP GET）でアクセスされた場合は、Web画面を返す
    if "Upgrade" not in request_headers or request_headers.get("Upgrade", "").lower() != "websocket":
        host_header = request_headers.get("Host", f"localhost:{PORT}")
        host_parts = host_header.split(":")
        server_host = host_parts[0]
        
        # SSL (https) かどうか判定 (X-Forwarded-Protoヘッダー対応)
        proto = request_headers.get("X-Forwarded-Proto", "http")
        is_ssl = (proto == "https" or PORT == 443)
        server_port = 443 if is_ssl else (int(host_parts[1]) if len(host_parts) > 1 else PORT)

        # 部屋数と人数集計
        room_count = len(rooms)
        client_count = 0
        for r in rooms.values():
            if r.get("host"): client_count += 1
            client_count += len(r.get("guests", set()))

        html = HTML_TEMPLATE.replace("{ROOM_COUNT}", str(room_count))\
                             .replace("{CLIENT_COUNT}", str(client_count))\
                             .replace("{SERVER_HOST}", server_host)\
                             .replace("{SERVER_PORT}", str(server_port))\
                             .replace("{SERVER_SSL}", "1" if is_ssl else "0")

        body = html.encode("utf-8")
        headers = [
            ("Content-Type", "text/html; charset=utf-8"),
            ("Content-Length", str(len(body))),
            ("Connection", "close"),
        ]
        return (http.HTTPStatus.OK, headers, body)
    return None

async def handler(websocket, path):
    parsed = urllib.parse.urlparse(path)
    query = urllib.parse.parse_qs(parsed.query)
    room_id = query.get("room", ["default"])[0]
    role = query.get("role", ["guest"])[0]

    print(f"[{room_id}] クライアント接続: role={role} (リモート: {websocket.remote_address})")

    if room_id not in rooms:
        rooms[room_id] = {"host": None, "guests": set()}

    if role == "host":
        rooms[room_id]["host"] = websocket
    else:
        rooms[room_id]["guests"].add(websocket)
        host_ws = rooms[room_id].get("host")
        if host_ws:
            try:
                await host_ws.send("GUEST_JOINED")
            except Exception:
                pass

    try:
        async for message in websocket:
            current_room = rooms.get(room_id)
            if not current_room:
                continue

            if websocket == current_room.get("host"):
                for guest in list(current_room["guests"]):
                    try:
                        await guest.send(message)
                    except Exception:
                        current_room["guests"].discard(guest)
            else:
                host_ws = current_room.get("host")
                if host_ws:
                    try:
                        await host_ws.send(message)
                    except Exception:
                        pass
    except Exception:
        pass
    finally:
        print(f"[{room_id}] 切断: role={role}")
        if room_id in rooms:
            if websocket == rooms[room_id].get("host"):
                rooms[room_id]["host"] = None
            rooms[room_id]["guests"].discard(websocket)
            if not rooms[room_id]["host"] and len(rooms[room_id]["guests"]) == 0:
                del rooms[room_id]
                print(f"[{room_id}] 部屋が空になったため削除しました")

async def main():
    print("=" * 60)
    print(f" TeamGame WebSocket Relay Server")
    print(f" 稼働ポート: {PORT}")
    print(f" Web管理画面URL: http://localhost:{PORT}")
    print("=" * 60)
    async with serve(handler, "0.0.0.0", PORT, process_request=process_request):
        await asyncio.Future()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nサーバーを停止しました")
