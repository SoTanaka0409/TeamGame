#include "Animation.h"
#include <cmath>

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
    // 標準モデルの初期マッピング (GreenCharacter等の標準アニメーション番号)
    m_motionMap[MotionType::Idle]  = 0;
    m_motionMap[MotionType::Walk]  = 1;
    m_motionMap[MotionType::Die]   = 2;
    m_motionMap[MotionType::Shoot] = 0; // 射撃専用モーションが無ければ待機を兼用
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
}

void Animation::SetMotionIndex(MotionType motion, int animIndex)
{
    m_motionMap[motion] = animIndex;
}

void Animation::Play(MotionType motion, bool loop, float speed)
{
    // 死亡モーション等の再生中は、他のモーションで上書きしない
    if (m_currentMotion == MotionType::Die && !m_isFinished && motion != MotionType::Die)
    {
        return;
    }

    // 既に同じモーションがループ再生中であれば継続
    if (m_currentMotion == motion && m_attachIndex != -1 && !m_isFinished && m_isLoop == loop)
    {
        m_playSpeed = speed;
        return;
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
    if (m_isFinished) return;

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
