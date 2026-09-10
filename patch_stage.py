import re

with open('TeamGame/Source/Stage/Stage.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

content = '#include "Camera.h"\n' + content

content = content.replace('float startDrawX = 960.0f - (playerWorldX / worldCellSize) * zoomCellSize;', 'float startDrawX = Camera::ScreenCenterX - (Camera::TargetWorldX / worldCellSize) * Camera::ZoomScale * worldCellSize;')
content = content.replace('float startDrawY = 540.0f - (playerWorldY / worldCellSize) * zoomCellSize;', 'float startDrawY = Camera::ScreenCenterY - (Camera::TargetWorldY / worldCellSize) * Camera::ZoomScale * worldCellSize;')
# wait, zoomCellSize is ZoomScale * worldCellSize. So zoomCellSize doesn't even need to be passed if we use Camera::ZoomScale * worldCellSize.
# Actually, I'll just change startDrawX to use Camera::WorldToScreenX(0)
content = content.replace('float startDrawX = Camera::ScreenCenterX - (Camera::TargetWorldX / worldCellSize) * Camera::ZoomScale * worldCellSize;', 'float startDrawX = Camera::WorldToScreenX(0.0f);')
content = content.replace('float startDrawY = Camera::ScreenCenterY - (Camera::TargetWorldY / worldCellSize) * Camera::ZoomScale * worldCellSize;', 'float startDrawY = Camera::WorldToScreenY(0.0f);')

content = content.replace('int drawX = static_cast<int>(startDrawX + x * zoomCellSize);', 'int drawX = static_cast<int>(Camera::WorldToScreenX(x * worldCellSize));')
content = content.replace('int drawY = static_cast<int>(startDrawY + y * zoomCellSize);', 'int drawY = static_cast<int>(Camera::WorldToScreenY(y * worldCellSize));')

# The pattern DrawBox / DrawGraph might use zoomCellSize which is Camera::ZoomScale * worldCellSize.
content = content.replace('int zSize = static_cast<int>(zoomCellSize);', 'int zSize = static_cast<int>(Camera::ZoomScale * worldCellSize);')

with open('TeamGame/Source/Stage/Stage.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
