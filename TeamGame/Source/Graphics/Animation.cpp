#include "Animation.h"
#include <cmath>
#include <string>
#include <cctype>

Animation::Animation(int modelHandle)
    : m_modelHandle(modelHandle),
      m_attachIndex(-1),
      m_currentMotion(MotionType::Idle),
      m_currentAnimIndex(-1),
      m_animTime(0.0f),
      m_totalAnimTime(0.0f),
      m_playSpeed(1.0f),
      m_isLoop(true),
      m_isFinished(false)
{
    // モデルのアニメーション構造を自動解析して正確にマッピング
    DetectAnimationsFromModel(modelHandle);
}

Animation::~Animation()
{
    if (m_modelHandle != -1 && m_attachIndex != -1)
    {
        MV1DetachAnim(m_modelHandle, m_attachIndex);
        m_attachIndex = -1;
    }
}

void Animation::SetModelHandle(int modelHandle)
{
    if (m_modelHandle != -1 && m_attachIndex != -1)
    {
        MV1DetachAnim(m_modelHandle, m_attachIndex);
        m_attachIndex = -1;
    }

    m_modelHandle = modelHandle;
    m_currentAnimIndex = -1;
    m_animTime = 0.0f;
    m_totalAnimTime = 0.0f;
    m_isFinished = false;

    // ハンドル再設定時にもアニメーション対応表を再構成
    DetectAnimationsFromModel(modelHandle);
}

void Animation::DetectAnimationsFromModel(int modelHandle)
{
    if (modelHandle == -1)
    {
        // デフォルトマッピング
        m_motionMap[MotionType::Idle]  = 0;
        m_motionMap[MotionType::Walk]  = 1;
        m_motionMap[MotionType::Die]   = 2;
        m_motionMap[MotionType::Shoot] = 0;
        return;
    }

    int animCount = MV1GetAnimNum(modelHandle);
    if (animCount <= 0)
    {
        m_motionMap[MotionType::Idle]  = 0;
        m_motionMap[MotionType::Walk]  = 1;
        m_motionMap[MotionType::Die]   = 2;
        m_motionMap[MotionType::Shoot] = 0;
        return;
    }

    int idleIdx = -1;
    int walkIdx = -1;
    int dieIdx = -1;
    int shootIdx = -1;

    for (int i = 0; i < animCount; ++i)
    {
        const TCHAR* rawName = MV1GetAnimName(modelHandle, i);
        std::string lowerName = "";
        if (rawName)
        {
            lowerName = rawName;
            for (char& c : lowerName) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        if (lowerName.find("attack") != std::string::npos || lowerName.find("shoot") != std::string::npos ||
            lowerName.find("shot") != std::string::npos || lowerName.find("fire") != std::string::npos)
        {
            shootIdx = i;
        }
        else if (lowerName.find("die") != std::string::npos || lowerName.find("death") != std::string::npos ||
                 lowerName.find("dead") != std::string::npos)
        {
            dieIdx = i;
        }
        else if (lowerName.find("idle") != std::string::npos || lowerName.find("idl") != std::string::npos ||
                 lowerName.find("stand") != std::string::npos || lowerName.find("wait") != std::string::npos)
        {
            idleIdx = i;
        }
        else if (lowerName.find("walk") != std::string::npos || lowerName.find("wa") != std::string::npos ||
                 lowerName.find("run") != std::string::npos || lowerName.find("move") != std::string::npos)
        {
            walkIdx = i;
        }
    }

    // 名前解析で見つからなかった場合の多重フォールバック
    if (animCount == 4)
    {
        // 5キャラクターのモデル構成: [0]Attack, [1]Die, [2]Idle, [3]Walk
        if (shootIdx == -1) shootIdx = 0;
        if (dieIdx == -1)   dieIdx   = 1;
        if (idleIdx == -1)  idleIdx  = 2;
        if (walkIdx == -1)  walkIdx  = 3;
    }
    else if (animCount == 3)
    {
        // GreenCharacter構成: [0]Die, [1]Idle, [2]Walk
        if (dieIdx == -1)   dieIdx   = 0;
        if (idleIdx == -1)  idleIdx  = 1;
        if (walkIdx == -1)  walkIdx  = 2;
        if (shootIdx == -1) shootIdx = idleIdx;
    }
    else
    {
        if (idleIdx == -1)  idleIdx  = 0;
        if (walkIdx == -1)  walkIdx  = (animCount > 1) ? 1 : idleIdx;
        if (dieIdx == -1)   dieIdx   = (animCount > 2) ? 2 : idleIdx;
        if (shootIdx == -1) shootIdx = (animCount > 3) ? 3 : idleIdx;
    }

    if (idleIdx != -1)  m_motionMap[MotionType::Idle]  = idleIdx;
    if (walkIdx != -1)  m_motionMap[MotionType::Walk]  = walkIdx;
    if (dieIdx != -1)   m_motionMap[MotionType::Die]   = dieIdx;
    if (shootIdx != -1) m_motionMap[MotionType::Shoot] = shootIdx;
    else if (idleIdx != -1) m_motionMap[MotionType::Shoot] = idleIdx;
}

void Animation::SetMotionIndex(MotionType motion, int animIndex)
{
    m_motionMap[motion] = animIndex;
}

void Animation::Reset()
{
    if (m_modelHandle != -1 && m_attachIndex != -1)
    {
        MV1DetachAnim(m_modelHandle, m_attachIndex);
        m_attachIndex = -1;
    }
    m_currentMotion = MotionType::Idle;
    m_currentAnimIndex = -1;
    m_animTime = 0.0f;
    m_totalAnimTime = 0.0f;
    m_playSpeed = 1.0f;
    m_isLoop = true;
    m_isFinished = false;
}

void Animation::Play(MotionType motion, bool loop, float speed)
{
    // 死亡モーションについて:
    // 既に死亡モーション再生中（または完了して倒れたままの状態）の場合、
    // 再びDieが呼ばれても絶対に最初から繰り返さない（最終フレームで倒れたまま静止を維持）
    if (m_currentMotion == MotionType::Die)
    {
        if (motion == MotionType::Die)
        {
            return; // 既に再生中または倒れ込み完了しているので絶対にリスタートしない！
        }
        // 死亡状態中は、リスポーン（Reset() / PlayIdle()）以外による他のモーション上書きを遮断
        if (motion != MotionType::Idle)
        {
            return;
        }
    }

    // 射撃・攻撃モーション再生中（単発）は、毎フレームのIdle/Walk呼び出しで即座に打ち消されないよう保護
    // （ただし死亡モーションが発生した場合は即座に死亡を優先）
    if (m_currentMotion == MotionType::Shoot && !m_isFinished &&
        motion != MotionType::Die && motion != MotionType::Shoot)
    {
        return;
    }

    // 既に同じモーションが再生中または完了している場合
    if (m_currentMotion == motion && m_attachIndex != -1)
    {
        // ループモーションまたは未終了モーションの場合はリスタートせず継続
        if (!m_isFinished && m_isLoop == loop)
        {
            m_playSpeed = speed;
            return;
        }
    }

    m_currentMotion = motion;

    int targetIndex = 0;
    auto it = m_motionMap.find(motion);
    if (it != m_motionMap.end())
    {
        targetIndex = it->second;
    }

    PlayByIndex(targetIndex, loop, speed);
}

void Animation::PlayByIndex(int animIndex, bool loop, float speed)
{
    if (m_modelHandle == -1) return;

    // 前のアニメーションを解除
    if (m_attachIndex != -1)
    {
        MV1DetachAnim(m_modelHandle, m_attachIndex);
        m_attachIndex = -1;
    }

    m_currentAnimIndex = animIndex;
    m_isLoop = loop;
    m_playSpeed = speed;
    m_isFinished = false;
    m_animTime = 0.0f;

    m_attachIndex = MV1AttachAnim(m_modelHandle, animIndex, -1, FALSE);
    if (m_attachIndex != -1)
    {
        m_totalAnimTime = MV1GetAttachAnimTotalTime(m_modelHandle, m_attachIndex);
        MV1SetAttachAnimTime(m_modelHandle, m_attachIndex, m_animTime);
    }
}

void Animation::Update(float deltaTime)
{
    if (m_modelHandle == -1 || m_attachIndex == -1) return;
    if (m_isFinished)
    {
        // 死亡モーションなど非ループモーションが終了した場合、最終フレームで確実に静止状態を維持
        if (!m_isLoop && m_totalAnimTime > 0.0f)
        {
            MV1SetAttachAnimTime(m_modelHandle, m_attachIndex, m_totalAnimTime);
        }
        return;
    }

    // DxLibのMV1アニメーション総時間(totalAnimTime)はフレーム単位(約60FPS)で管理されるため、
    // 秒単位のdeltaTime(例: 1/60s = 0.0166s)が渡された場合は60倍してフレーム単位に進める
    float advance = (deltaTime < 0.5f) ? (deltaTime * 60.0f) : deltaTime;
    m_animTime += m_playSpeed * advance;

    if (m_totalAnimTime > 0.0f)
    {
        if (m_animTime >= m_totalAnimTime)
        {
            if (m_isLoop)
            {
                m_animTime = std::fmod(m_animTime, m_totalAnimTime);
            }
            else
            {
                m_animTime = m_totalAnimTime;
                m_isFinished = true;
            }
        }
    }

    MV1SetAttachAnimTime(m_modelHandle, m_attachIndex, m_animTime);
}
