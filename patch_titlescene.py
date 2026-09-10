import re

with open('TeamGame/Source/Scenes/TitleScene.h', 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('enum class TitleState { MAIN, SETTINGS, WAITING, JOINING_LAN };', 'enum class TitleState { MAIN, MODE_SELECT, JOIN_SELECT, SETTINGS, WAITING, JOINING_LAN };')

with open('TeamGame/Source/Scenes/TitleScene.h', 'w', encoding='utf-8') as f:
    f.write(content)
