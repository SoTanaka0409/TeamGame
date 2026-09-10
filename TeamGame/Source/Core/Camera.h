#pragma once
#include "Vector2.h"

class Camera
{
public:
    static float TargetWorldX;
    static float TargetWorldY;
    static float ScreenCenterX;
    static float ScreenCenterY;
    static float ZoomScale;

    static float WorldToScreenX(float worldX)
    {
        return ScreenCenterX + (worldX - TargetWorldX) * ZoomScale;
    }
    static float WorldToScreenY(float worldY)
    {
        return ScreenCenterY + (worldY - TargetWorldY) * ZoomScale;
    }
    static float ScreenToWorldX(float screenX)
    {
        return TargetWorldX + (screenX - ScreenCenterX) / ZoomScale;
    }
    static float ScreenToWorldY(float screenY)
    {
        return TargetWorldY + (screenY - ScreenCenterY) / ZoomScale;
    }
};
