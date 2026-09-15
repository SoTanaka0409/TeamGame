#pragma once
#include <string>
#include "DxLib.h"

class Model
{
private:
    int handle;
    int currentAnimAttachIndex;
    float currentAnimTime;
    float totalAnimTime;

public:
    // 通常の読み込み（ファイルから）
    Model(const std::string& path) : currentAnimAttachIndex(-1), currentAnimTime(0.0f), totalAnimTime(0.0f)
    {
        handle = MV1LoadModel(path.c_str());
    }

    // 複製用（ResourceManagerから渡される）
    Model(int rawHandle) : currentAnimAttachIndex(-1), currentAnimTime(0.0f), totalAnimTime(0.0f)
    {
        // 既存のモデルハンドルから複製を作成（メモリと読み込み時間を節約）
        if (rawHandle != -1) {
            handle = MV1DuplicateModel(rawHandle);
        } else {
            handle = -1;
        }
    }

    ~Model()
    {
        if (handle != -1) {
            MV1DeleteModel(handle);
            handle = -1;
        }
    }

    // コピー禁止
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    void Draw() const
    {
        if (handle != -1) {
            MV1DrawModel(handle);
        }
    }

    void SetPosition(float x, float y, float z)
    {
        if (handle != -1) {
            MV1SetPosition(handle, VGet(x, y, z));
        }
    }

    void SetRotation(float x, float y, float z) // ラジアン指定
    {
        if (handle != -1) {
            MV1SetRotationXYZ(handle, VGet(x, y, z));
        }
    }

    void SetScale(float x, float y, float z)
    {
        if (handle != -1) {
            MV1SetScale(handle, VGet(x, y, z));
        }
    }

    // アニメーションの切り替え
    void PlayAnimation(int animIndex)
    {
        if (handle == -1) return;
        
        // 前のアニメーションを解除
        if (currentAnimAttachIndex != -1) {
            MV1DetachAnim(handle, currentAnimAttachIndex);
        }

        currentAnimAttachIndex = MV1AttachAnim(handle, animIndex, -1, FALSE);
        totalAnimTime = MV1GetAttachAnimTotalTime(handle, currentAnimAttachIndex);
        currentAnimTime = 0.0f;
        MV1SetAttachAnimTime(handle, currentAnimAttachIndex, currentAnimTime);
    }

    // アニメーションの時間を進める（毎フレーム呼ぶ）
    void UpdateAnimation(float speed = 1.0f)
    {
        if (handle != -1 && currentAnimAttachIndex != -1) {
            currentAnimTime += speed;
            // アニメーションが最後まで到達したらループさせる
            if (currentAnimTime >= totalAnimTime) {
                currentAnimTime = fmod(currentAnimTime, totalAnimTime);
            }
            MV1SetAttachAnimTime(handle, currentAnimAttachIndex, currentAnimTime);
        }
    }

    int GetHandle() const { return handle; }
};
