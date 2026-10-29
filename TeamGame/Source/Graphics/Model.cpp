#include "Model.h"
#include <cmath>
#include <vector>
#include <unordered_map>

namespace
{
    // 同一パスの3Dモデルを毎回ディスクから読み込まず、VRAMバッファを共有するMV1DuplicateModel用キャッシュ
    std::unordered_map<std::string, int> s_baseModelCache;
}

Model::Model(const std::string& path)
    : m_handle(-1),
      m_position(VGet(0.0f, 0.0f, 0.0f)),
      m_rotation(VGet(0.0f, 0.0f, 0.0f)),
      m_scale(VGet(DEFAULT_SCALE, DEFAULT_SCALE, DEFAULT_SCALE)),
      m_facingAngle(0.0f),
      m_pitchAngle(0.0f),
      m_tiltAngle(24.0f * (DX_PI_F / 180.0f)),
      m_matrixDirty(true),
      m_animation(nullptr)
{
    int baseHandle = -1;
    auto it = s_baseModelCache.find(path);
    if (it != s_baseModelCache.end() && it->second != -1)
    {
        baseHandle = it->second;
    }
    else
    {
        baseHandle = MV1LoadModel(path.c_str());
        if (baseHandle == -1)
        {
            // 実行時のカレントディレクトリの違いを吸収するためのフォールバック探索
            std::string subPath = path;
            if (subPath.rfind("Resource/", 0) == 0) {
                subPath = subPath.substr(9);
            }
            std::vector<std::string> fallbacks = {
                "Resource/" + subPath,
                "TeamGame/Resource/" + subPath,
                "TeamGame/TeamGame/Resource/" + subPath,
                "../Resource/" + subPath,
                "../../Resource/" + subPath,
                "../TeamGame/Resource/" + subPath,
                "../../TeamGame/Resource/" + subPath,
                "../../TeamGame/TeamGame/Resource/" + subPath,
                "Resource/" + path,
                "TeamGame/" + path,
                "TeamGame/TeamGame/" + path,
                "../" + path,
                "../../" + path,
                "../TeamGame/" + path,
                "../../TeamGame/" + path,
                "../../TeamGame/TeamGame/" + path
            };
            for (const auto& fb : fallbacks)
            {
                baseHandle = MV1LoadModel(fb.c_str());
                if (baseHandle != -1) break;
            }
        }
        if (baseHandle != -1)
        {
            s_baseModelCache[path] = baseHandle;
        }
    }

    if (baseHandle != -1)
    {
        m_handle = MV1DuplicateModel(baseHandle);
        if (m_handle == -1)
        {
            m_handle = baseHandle;
        }
    }

    m_animation = std::make_unique<Animation>(m_handle);

    if (m_handle != -1)
    {
        SetupMaterials();
        m_matrixDirty = true;
        PlayIdle();
    }
}

Model::Model(int rawHandle)
    : m_handle(-1),
      m_position(VGet(0.0f, 0.0f, 0.0f)),
      m_rotation(VGet(0.0f, 0.0f, 0.0f)),
      m_scale(VGet(DEFAULT_SCALE, DEFAULT_SCALE, DEFAULT_SCALE)),
      m_facingAngle(0.0f),
      m_pitchAngle(0.0f),
      m_tiltAngle(24.0f * (DX_PI_F / 180.0f)),
      m_matrixDirty(true),
      m_animation(nullptr)
{
    if (rawHandle != -1)
    {
        m_handle = MV1DuplicateModel(rawHandle);
    }
    m_animation = std::make_unique<Animation>(m_handle);

    if (m_handle != -1)
    {
        SetupMaterials();
        m_matrixDirty = true;
        PlayIdle();
    }
}

void Model::SetupMaterials()
{
    if (m_handle == -1) return;
    int matNum = MV1GetMaterialNum(m_handle);
    for (int m = 0; m < matNum; ++m)
    {
        const char* name = MV1GetMaterialName(m_handle, m);
        COLOR_F diff = MV1GetMaterialDifColor(m_handle, m);

        if (name && std::string(name) == "Mat_Body")
        {
            // 体のベースカラーを鮮やかに補正＆スペキュラ強調
            MV1SetMaterialDifColor(m_handle, m, GetColorF(0.28f, 0.68f, 0.16f, 1.0f));
            MV1SetMaterialAmbColor(m_handle, m, GetColorF(0.22f, 0.50f, 0.12f, 1.0f));
            MV1SetMaterialSpcColor(m_handle, m, GetColorF(0.4f, 0.4f, 0.4f, 1.0f));
        }
        else if (name && std::string(name) == "Mat_Visor")
        {
            // バイザーの黒光り感を強調
            MV1SetMaterialDifColor(m_handle, m, GetColorF(0.08f, 0.10f, 0.08f, 1.0f));
            MV1SetMaterialAmbColor(m_handle, m, GetColorF(0.05f, 0.05f, 0.05f, 1.0f));
            MV1SetMaterialSpcColor(m_handle, m, GetColorF(0.8f, 0.8f, 0.8f, 1.0f));
        }
        else
        {
            MV1SetMaterialAmbColor(m_handle, m, GetColorF(diff.r * 0.8f, diff.g * 0.8f, diff.b * 0.8f, 1.0f));
        }
    }
}

void Model::UpdateMatrix() const
{
    if (m_handle == -1 || !m_matrixDirty) return;

    // 1. スケール
    MATRIX mScale = MGetScale(m_scale);
    // 2. エイム方向へのY軸回転
    MATRIX mRotY = MGetRotY(m_rotation.y);
    // 3. 画面手前側（カメラ方向）への2.5Dチルト傾斜 (+X軸周りの回転)
    MATRIX mTilt = MGetRotX(m_tiltAngle);

    // 行列の合成: モデル向き(mRotY) -> 画面手前チルト(mTilt) -> スケール
    MATRIX mWorld = MMult(mRotY, mTilt);
    mWorld = MMult(mScale, mWorld);
    // 4. 足元位置への配置
    MATRIX mTrans = MGetTranslate(m_position);
    mWorld = MMult(mWorld, mTrans);

    MV1SetMatrix(m_handle, mWorld);
    m_matrixDirty = false;
}

Model::~Model()
{
    m_animation.reset();
    if (m_handle != -1)
    {
        MV1DeleteModel(m_handle);
        m_handle = -1;
    }
}

void Model::SetPosition(float x, float y, float z)
{
    m_position = VGet(x, y, z);
    m_matrixDirty = true;
}

void Model::SetPosition2D(const Vector2& pos, float z)
{
    SetPosition(pos.x, pos.y, z);
}

void Model::SetScreenPosition(float screenX, float screenY)
{
    int sw = 1920, sh = 1080;
    GetDrawScreenSize(&sw, &sh);
    SetPosition(screenX, 0.0f, static_cast<float>(sh) - screenY);
}

void Model::SetRotation(float rx, float ry, float rz)
{
    m_rotation = VGet(rx, ry, rz);
    m_pitchAngle = rx;
    m_facingAngle = ry + (DX_PI_F / 2.0f);
    m_matrixDirty = true;
}

void Model::SetFacingDir(const Vector2& dir)
{
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.0001f)
    {
        float ang = std::atan2(dir.y, dir.x);
        float rotY = ang - (DX_PI_F / 2.0f);
        m_rotation.y = rotY;
        m_facingAngle = ang;
        m_matrixDirty = true;
    }
}

void Model::SetTiltAngle(float tilt)
{
    m_tiltAngle = tilt;
    m_matrixDirty = true;
}

void Model::SetPitchAngle(float pitch)
{
    m_pitchAngle = pitch;
    m_rotation.x = pitch;
    m_matrixDirty = true;
}

void Model::SetScale(float sx, float sy, float sz)
{
    m_scale = VGet(sx, sy, sz);
    m_matrixDirty = true;
}

void Model::SetScaleUniform(float s)
{
    SetScale(s, s, s);
}

void Model::PlayIdle(float speed)
{
    PlayMotion(MotionType::Idle, true, speed);
}

void Model::PlayWalk(float speed)
{
    PlayMotion(MotionType::Walk, true, speed);
}

void Model::PlayShoot(float speed)
{
    PlayMotion(MotionType::Shoot, false, speed);
}

void Model::PlayDie(float speed)
{
    PlayMotion(MotionType::Die, false, speed);
}

void Model::PlayMotion(MotionType motion, bool loop, float speed)
{
    if (m_animation)
    {
        m_animation->Play(motion, loop, speed);
    }
}

MotionType Model::GetCurrentMotion() const
{
    return m_animation ? m_animation->GetCurrentMotion() : MotionType::Idle;
}

bool Model::IsMotionFinished() const
{
    return m_animation ? m_animation->IsFinished() : true;
}

void Model::Update(float deltaTime)
{
    if (m_animation)
    {
        m_animation->Update(deltaTime);
    }
}

void Model::Draw() const
{
    DrawWithOpacity(1.0f);
}

void Model::DrawWithOpacity(float opacity) const
{
    if (m_handle != -1 && opacity > 0.001f)
    {
        int sw = 1920, sh = 1080;
        GetDrawScreenSize(&sw, &sh);

        // 画面外の3Dキャラクターモデルはスキニング・描画をスキップして高速化
        if (m_position.x < -180.0f || m_position.x > static_cast<float>(sw) + 180.0f ||
            m_position.z < -180.0f || m_position.z > static_cast<float>(sh) + 180.0f)
        {
            return;
        }

        UpdateMatrix();

        float centerX = static_cast<float>(sw) / 2.0f;
        float centerY = static_cast<float>(sh) / 2.0f;

        // 地面(X-Z平面)に対して直立したキャラクターを真上(+Y)から見下ろす正射影（Ortho）カメラ
        SetCameraPositionAndTargetAndUpVec(
            VGet(centerX, 2000.0f, centerY),
            VGet(centerX, 0.0f, centerY),
            VGet(0.0f, 0.0f, 1.0f)
        );
        SetupCamera_Ortho(static_cast<float>(sh));

        SetUseLighting(TRUE);
        SetGlobalAmbientLight(GetColorF(0.75f, 0.75f, 0.75f, 1.0f));
        ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));

        if (opacity < 0.999f)
        {
            MV1SetOpacityRate(m_handle, opacity);
            MV1DrawModel(m_handle);
            MV1SetOpacityRate(m_handle, 1.0f);
        }
        else
        {
            MV1DrawModel(m_handle);
        }
    }
}


