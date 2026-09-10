import re

with open('TeamGame/Source/Characters/Enemy.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

content = re.sub(r'        screenX = 960\.0f \+ \(position\.x - pPos\.x\) \* zoomScale;\n        screenY = 540\.0f \+ \(position\.y - pPos\.y\) \* zoomScale;', '        screenX = Camera::WorldToScreenX(position.x);\n        screenY = Camera::WorldToScreenY(position.y);', content)

with open('TeamGame/Source/Characters/Enemy.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
