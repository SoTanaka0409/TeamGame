# Blind ZOne リファクタリング引継ぎメモ（自宅作業・再開用）

作成日時：2026-11-27 15:01  
対象リポジトリ：`TeamGame`（現在ブランチ：`main` / HEAD：`444658c`）

---

## 1. 現在の状態（ここまでは完了＆ビルド正常）

- **パフォーマンス軽量化（1〜3）完了**
  - ライトマスクの1Dレイキャスト＋240×135 ARGB8 `SoftImage` 1DrawCall化
  - ステージ描画の範囲カリング・影バッチ化・3D草むらLOD絞り込み・`Floor.png` / `Water.png` の512px不透過化・岩ベイク16枚化
  - Releaseログ抑止・GPU優先エクスポート・`Sleep(1)` 省CPU待機・読み込み失敗キャッシュ
- **計測用コード（`PerfProfiler`）は削除済み**
- **Phase 1-A（`Player` 責務分離 ＋ 武器 `std::unique_ptr` 化）完了**
  - 新設：`Source/Graphics/LightMaskRenderer.{h,cpp}`（暗闇マスク描画＋`IsWorldPosInLight`視界判定）
  - 新設：`Source/UI/PlayerHudRenderer.{h,cpp}`（左下HUD・血しぶき警告・HighRisk敵インジケータ・オートエイムレティクル、RAII管理）
  - 改修：`Player.{h,cpp}`（1,686行 → 1,021行）、`Enemy.{h,cpp}`、`CharacterFactory.{h,cpp}`（武器所有権を `std::unique_ptr<Weapon>` に統一）
  - 改修：`TeamGame.vcxproj` / `TeamGame.vcxproj.filters` 同期済み
- **ビルド確認**：`Debug|x64`・`Release|x64` ともに **0エラー・0警告**（未コミット状態なので、自宅へ持ち帰る前に Git commit & push またはフォルダごとコピーしてください）

---

## 2. 設計方針・作業ルール（厳守）

1. **ゲーム性は一切変えない**（移動速度・当たり判定・AI挙動・ダメージ・演出・見た目は現状と完全一致させる）
2. **保守性・拡張性重視のモダンC++ / OOP設計**
   - 単一責任の原則（ロジック／描画／入力／UIを分離）
   - 継承よりコンポジション優先（部品クラスを `std::unique_ptr` で所有）
   - 所有権の明確化（生 `new` / `delete` を減らし `std::unique_ptr` + RAII へ）
   - `template` 活用と型安全化で `dynamic_cast`（現在43箇所）と文字列判定を削減
3. **1フェーズごとに進める**
   - 1フェーズ実装 → `Debug|x64` と `Release|x64` をビルド確認（0エラー・0警告）→ 動作確認 → 次へ
   - ※ `TeamGame.vcxproj` の PostBuildEvent が `TeamGame.exe` を上書きするため、必ず **`Debug|x64` を先に、`Release|x64` を後に** ビルドすること
4. **プロジェクト規約（`GEMINI.md` 準拠）**
   - 新規・編集する `.h` / `.cpp` / `.md` は **UTF-8（BOM付き）＋ 改行コード CRLF（`\r\n`）** で保存する
   - 新規ファイルは必ず `TeamGame/TeamGame.vcxproj` と `TeamGame/TeamGame.vcxproj.filters` の両方へ登録する
   - フォルダ構成は既存構造（`Source/Characters`, `Source/Graphics`, `Source/UI`, `Source/Stage`, `Source/Core`, `Source/Managers`, `Source/Weapons`）を活かしつつ役割別に整理する

---

## 3. これからの作業ロードマップ（次は Phase 1-B から）

### ▶ Phase 1-B：`Enemy`（1,561行）の責務分離【次回すぐここから】
- **目的**：AIロジック・戦術定義・描画が1ファイルに混在している `Enemy.cpp` を責務ごとに分割
- **新設予定ファイル**
  1. `Source/Graphics/EnemyRenderer.{h,cpp}`
     - 担当：足元チームカラー円、3Dモデル描画、頭上ネームタグ＆HPバー、特攻オーラ、パニック星エフェクト、警戒リング／射程円／弾道予測線、視界コーン、視線ライン
     - `Enemy::Draw()` は `EnemyRenderer::Draw(*this)` への委譲だけにする（約260行削減）
  2. `Source/Characters/EnemyTactics.{h,cpp}`（または `EnemyAI.{h,cpp}`）
     - 担当：`BotPersonality` / `WeaponAITactic` 構築テーブル（`CreatePersonalityForCharacter(id)` / `CreateWeaponTactic(weaponName)`）、`FindCoverPosition`（遮蔽物探索）、`PickDispersalTarget`（区画散開先選定）
     - `Enemy` は「キャラ本体・状態保持・Update制御」に集中させ、性格や武器戦術の追加を拡張しやすくする
- **ポイント**
  - `Enemy.cpp` の1464行目付近に途中記述されている重複 `#include "SceneManager.h"` / `"Scene.h"` も整理
  - `Enemy::GetFacingDir()`, `GetCurrentStage()`, `GetCellSize()`, `IsHomingSuicide()`, `GetModel()` 等の読み取り用 getter を `Enemy.h` に追加

---

### ▶ Phase 1-C：`Stage`（1,078行）の責務分離
- **目的**：純粋なマップデータ・衝突/遮光判定と、DXLibの描画・3Dベイク処理を分離
- **新設予定ファイル**
  1. `Source/Graphics/StageImposterCache.{h,cpp}`
     - 担当：3D岩（4種×2角度×2種＝16枚）と3D草むら（4種）の初回オフスクリーンベイク（`MakeScreen`）と取得・終了時解放（RAII / `Release()`）
     - 現在 `Stage.cpp` の無名名前空間にある `static` 配列群をクラス化
  2. `Source/Graphics/StageRenderer.{h,cpp}`
     - 担当：`DrawFitToArea` / `DrawZoomCamera` / `DrawForegroundGrass`（画面内タイルカリング、不透過ベース描画、影バッチ描画、3D草ハイブリッドLOD）
- **`Stage.{h,cpp}` に残す責務**
  - グリッド配列、`GetCell`/`SetCell`、`IsSolidWall`/`IsBulletBlockingWall`/`IsLightBlockingWall`、`ResolveCollision`、スポーン座標管理のみ（約250行にスリム化）

---

### ▶ Phase 1-D：`GameScene`（1,038行）の責務分離
- **目的**：試合進行・カメラ制御と、戦闘HUDおよびポーズ/設定オーバーレイUIを分離
- **新設予定ファイル**
  1. `Source/UI/GameHudView.{h,cpp}`
     - 担当：左上Escバッジ、上部中央キルバー＆対向ゲージ、バトルロワイヤル錯乱バナー、薄明/音波探知通知バナー、イントロ文字表示
  2. `Source/UI/PauseMenuOverlay.{h,cpp}`
     - 担当：ポーズ・設定・操作説明のカーソル状態・入力更新・描画一式（戻り値で「再開 / やり直し / タイトル / 終了」アクションを `GameScene` へ通知）
- **ポイント**
  - `GameScene` は「試合ステート・イベントタイマー・カメラ・シーン遷移」に専念

---

### ▶ Phase 2：音・AI・UIの疎結合化（イベント仲介の導入）
- **現状の課題**：`SoundManager::Play3D()` が音声再生だけでなく `Enemy::OnHearGunshot()` と `SoundVisualizer::AddSound()` を直接呼んでおり、音声システムがゲームルールに逆依存している
- **新設予定ファイル**：`Source/Core/WorldSoundEvent.{h,cpp}`
  - 構造体 `WorldSoundEvent { key, pos, maxDistance, baseVolume, sourceTeamId, visualType }`
  - `WorldSoundEvent::Emit(...)` が以下3系統へ並列配信：
    1. `SoundManager::Play3D(...)`（純粋な空間オーディオ再生のみ）
    2. 敵AIの聴覚通知（透明化チェック込み）
    3. `SoundVisualizer::AddSound(...)`（ライト範囲外チェック込み）
- **効果**：`SoundManager` から `Enemy.h` / `Player.h` / `GameScene.h` / `SoundVisualizer.h` の依存を完全除去

---

### ▶ Phase 3：`ObjectManager` のテンプレート化・タグ/ID整理（`dynamic_cast` 削減）
- **現状の課題**：コード全体で `dynamic_cast` が **43箇所** あり、`HitFeedbackManager` / `MatchManager` では名前文字列 `"プレイヤー"` での比較が残っている
- **改修内容**
  1. `ObjectManager.h` に **テンプレート検索・走査API** を追加：
     - `template <typename T, typename Func> void ForEach(Func&& fn) const;`
     - `template <typename T> T* FindFirst() const;`
     - さらに `Character` / `Enemy` / `Player` への高速アクセス（`GetPlayer()` や `ObjectTag` 併用での型フィルタ）を用意し、全件 `dynamic_cast` ループを大幅削減
  2. `Object2D` に一意の `uint32_t entityId`、明確な `enum class TeamId { Neutral = -1, Ally = 0, Enemy = 1 }`（段階導入）を整備
  3. 文字列比較 `"プレイヤー"` を `player == attacker` または `isLocalPlayer` フラグ／`entityId` 比較へ置き換え
  4. `ObjectManager` のオブジェクト所有を `std::vector<std::unique_ptr<Object2D>>` へ移行可能か検討・実施（生 `new`/`delete` のさらなる削減）

---

### ▶ Phase 4：リソース管理・パス解決の一本化
- **現状の課題**：`FileExistsUtf8` と複数パス探索が `Scene.cpp` / `StageManager.cpp` / `SoundManager.cpp` / `Model.cpp` / `CharacterSelectScene.cpp` に重複し、`ResourceManager.h` が未使用のまま残っている
- **新設・整理予定**
  - `Source/Core/AssetPathResolver.{h,cpp}`：UTF-8存在確認と相対パス解決を1箇所に集約
  - `ResourceManager` を実体化または `Scene::Load2DImage` / `Model` キャッシュと統合し、画像・3Dモデルのキャッシュと終了時解放を一元管理

---

### ▶ Phase 5：小物クラス・重複コードの仕上げ整理
- `EffectManager.h`（413行）/ `EffectManager.cpp`（590行）の9種エフェクトをカテゴリ別に分割（例：`Source/Effects/CombatEffects.{h,cpp}`, `Source/Effects/WaveEffects.{h,cpp}`）
- `Bullet` と `EnemyBullet` の共通移動・壁衝突ロジックを基底クラスまたはヘルパーへ整理
- 未使用の `ResultScene.{h,cpp}` 等の整理要否を確認

---

## 4. 自宅でAIに貼り付けてすぐ再開するためのプロンプト（コピペ用）

自宅の環境で新しいチャットを開いたら、以下をそのまま貼り付ければすぐ続きから再開できます：

```text
TeamGame/REFACTORING_HANDOFF.md を読んで現在の状況と設計方針を把握してください。
ゲーム性は一切変えず、保守性・拡張性重視（単一責任・unique_ptr・template・dynamic_cast削減・UTF-8 BOM+CRLF・vcxproj同期）で、
次は Phase 1-B（Enemy の責務分離）から実装し、Debug|x64 → Release|x64 の順でビルド確認まで進めてください。
```
