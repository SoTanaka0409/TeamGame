# Project Coding Guidelines & Development Note (チーム開発ガイドライン)

このファイルは、プロジェクト `TeamGame` のコード規則・命名規則・設計の注意点をまとめたドキュメントです。
Antigravity などの AI アシスタントやチームメンバーが新しい PC 環境で作業する際に、このガイドラインを自動的に読み込んで従うための指示書として機能します。

---

## 1. プロジェクト構造 (Project Structure)
- **ソリューションファイル**: `TeamGame.sln` (Visual Studio 2022 / x86 プラットフォーム)
- **ソースコードディレクトリ**: `TeamGame/Source/`
  - `Core/` : エントリポイント (`main.cpp`), `Camera.cpp`
  - `Characters/` : `Character`, `Player`, `Enemy`, `Status`, `PathfindingComponent`
  - `Weapons/` : `Bullet`, `Handgun`, `Shotgun`, `SniperRifle`, `EnemyBullet`
  - `Managers/` : 各種マネージャー (`ObjectManager`, `ColliderManager`, `SceneManager`, `SoundManager`, `DebugManager`, `EffectManager`, `InputManager`, `WeaponManager`, `NetworkManager`)
  - `Scenes/` : ゲームシーン (`GameScene`, `TitleScene`, `ResultScene`, `ClearScene`, `GameOverScene`)
  - `Colliders/` : 判定処理 (`Collider`, `CircleCollider`, `RectCollider`)
  - `Stage/` : ステージ生成・管理 (`Stage`, `StageManager`, `StageGenerator`)
  - `Objects/` : オブジェクト基礎 (`Object2D`, `Item`)

---

## 2. 文字コード & 改行コード (Encoding & Line Endings)
- **文字コード**: UTF-8 with BOM (`utf-8-sig`)
  - **重要**: Visual C++ (MSVC) で日本語コメントの文字化け（C4828 警告等）を防ぐため、すべての `.h`, `.cpp` ファイルは **BOM 付き UTF-8** で保存してください。
- **改行コード**: CRLF (`\r\n`)

---

## 3. 命名規則 (Naming Conventions)
- **クラス名**: PascalCase (`Player`, `GameScene`, `ColliderManager`)
- **メソッド名**: PascalCase (`SpawnEnemiesRandomly`, `Update`, `Draw`, `GetInstance`)
- **メンバ変数**: camelCase (`team0Kills`, `maxKills`, `playerWorldX`, `cellSize`)
- **定数 / マクロ**: ALL_CAPS または PascalCase

---

## 4. コメント・ドキュメント形式 (Documentation Standards)
- 原則として **Doxygen 形式** の日本語コメントを記述します。
```cpp
/**
 * @brief 関数の概要説明
 * @param paramName 引数の説明
 * @return 戻り値の説明
 * @details 詳細な処理フローや注意事項
 */
```

---

## 5. ゲーム設計 & チームID仕様 (Game Architecture & Team Rules)
- **チームID (teamId)**:
  - `teamId = 0` : プレイヤーおよび味方 Bot (青色表示 / `ENEMY KILLS` カウント増加)
  - `teamId = 1` : 敵 Bot (赤色表示 / `ALLY LOSSES` カウント増加)
- **UI表示仕様**:
  - `team0Kills` (味方チームが倒した敵の数) -> `ENEMY KILLS: X/10` (画面中央上・青バー)
  - `team1Kills` (敵チームに倒された味方の数) -> `ALLY LOSSES: X/10` (画面中央上・赤バー)
- **オブジェクト管理**:
  - 動的に生成するキャラ・ボット・弾・アイテムは、生成後に `objectManager->AddObject(obj)` を呼んで登録すること。

---

## 6. ビルド & 動作確認手順 (Build & Verification)
- Visual Studio 2022 で `TeamGame.sln` を開いてビルド (x86 Debug)。
- またはコマンドラインビルド (MSBuild):
  ```cmd
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" && msbuild TeamGame.sln /p:Configuration=Debug /p:Platform=x86
  ```
- ビルド時に 0 Error であることを確認してからコミット・プッシュしてください。

---

## 7. AI アシスタントへの指示 (Instructions for AI Tools / Antigravity)
- ソースコード編集時は必ず **UTF-8 with BOM** で出力すること。
- ゲームロジックを修正・リファクタリングする際は、事前にビルドを通して既存機能が破損しないか検証すること。
- コメントや変数を統一する作業の前後で、必ず動作する状態であることを確認すること。
