#include "Model.h"
#include <cmath>
#include <vector>

Model::Model(const std::string& path)
    : m_handle(-1),
      m_position(VGet(0.0f, 0.0f, 0.0f)),
      m_rotation(VGet(0.0f, 0.0f, 0.0f)),
      m_scale(VGet(DEFAULT_SCALE, DEFAULT_SCALE, DEFAULT_SCALE)),
      m_facingAngle(0.0f),
      m_pitchAngle(0.0f),
      m_tiltAngle(24.0f * (DX_PI_F / 180.0f)),
      m_animation(nullptr)
{
    m_handle = MV1LoadModel(path.c_str());
    if (m_handle == -1)
    {
        // 実行時のカレントディレクトリの違いを吸収するためのフォールバック探索
        std::vector<std::string> fallbacks = {
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
            m_handle = MV1LoadModel(fb.c_str());
            if (m_handle != -1) break;
        }
    }

    m_animation = std::make_unique<Animation>(m_handle);

    if (m_handle != -1)
    {
        SetupMaterials();
        UpdateMatrix();
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
        UpdateMatrix();
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

void Model::UpdateMatrix()
{
    if (m_handle == -1) return;

    // 1. スケール
    MATRIX mScale = MGetScale(m_scale);
    // 2. エイム方向へのY軸回転
    MATRIX mRotY = MGetRotY(m_rotation.y);
    // 3. 画面手前側（カメラ方向）への2.5Dチルト傾斜 (+X軸周りの回転)
    MATRIX mTilt = MGetRotX(m_tiltAngle);

    // 行列の合成: モデル向き(mRotY) -> 画面手前チルト(mTilt) -> スケール
    // これにより、どの方向を向いても常に画面手前側に頭部・胸部が一定角度傾き、重心位置（足元原点）がブレない
    MATRIX mWorld = MMult(mRotY, mTilt);
    mWorld = MMult(mScale, mWorld);
    // 4. 足元位置への配置
    MATRIX mTrans = MGetTranslate(m_position);
    mWorld = MMult(mWorld, mTrans);

    MV1SetMatrix(m_handle, mWorld);
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
    UpdateMatrix();
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
    UpdateMatrix();
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
        UpdateMatrix();
    }
}

void Model::SetTiltAngle(float tilt)
{
    m_tiltAngle = tilt;
    UpdateMatrix();
}

void Model::SetPitchAngle(float pitch)
{
    m_pitchAngle = pitch;
    m_rotation.x = pitch;
    UpdateMatrix();
}

void Model::SetScale(float sx, float sy, float sz)
{
    m_scale = VGet(sx, sy, sz);
    UpdateMatrix();
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
    if (m_handle != -1)
    {
        int sw = 1920, sh = 1080;
        GetDrawScreenSize(&sw, &sh);
        float centerX = static_cast<float>(sw) / 2.0f;
        float centerY = static_cast<float>(sh) / 2.0f;

        // 地面(X-Z平面)に対して直立したキャラクターを真上(+Y)から見下ろす正射影（Ortho）カメラ
        // UpVec=(0,0,1) により、画面上方向(+Y)が3D+Z、画面右方向(+X)が3D+Xに対応し、
        // 2Dスクリーン座標系 (0,0)〜(sw,sh) と 3D (0,0,sh)〜(sw,0,0) がピクセル単位で1:1対応
        SetCameraPositionAndTargetAndUpVec(
            VGet(centerX, 2000.0f, centerY),
            VGet(centerX, 0.0f, centerY),
            VGet(0.0f, 0.0f, 1.0f)
        );
        SetupCamera_Ortho(static_cast<float>(sh));

        SetUseLighting(TRUE);
        SetGlobalAmbientLight(GetColorF(0.75f, 0.75f, 0.75f, 1.0f));
        // 斜め手前上からのディレクショナルライト（顔・肩・バイザーに陰影とハイライト）
        ChangeLightTypeDir(VGet(0.3f, -0.8f, -0.5f));
        MV1DrawModel(m_handle);
    }
}
