import os
import shutil
import glob
import re
import xml.etree.ElementTree as ET

PROJ_ROOT = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame'
SRC_DIR = os.path.join(PROJ_ROOT, 'Source')
OUTER_SRC = r'C:\Users\student\Desktop\TeamGame\TeamGame\Source'

dirs = ['Core', 'Managers', 'Scenes', 'Objects', 'Characters', 'Weapons', 'Colliders', 'Stage']
for d in dirs:
    os.makedirs(os.path.join(SRC_DIR, d), exist_ok=True)

# 1. Rename and fix Stage prototype files
stage_files = ['Stage.h', 'Stage.cpp', 'StageGenerator.h', 'StageGenerator.cpp']
for sf in stage_files:
    if os.path.exists(os.path.join(SRC_DIR, sf)):
        shutil.move(os.path.join(SRC_DIR, sf), os.path.join(SRC_DIR, 'Stage', sf))
        
if os.path.exists(os.path.join(SRC_DIR, 'Object2D.h')):
    shutil.move(os.path.join(SRC_DIR, 'Object2D.h'), os.path.join(SRC_DIR, 'Stage', 'StageTypes.h'))

if os.path.exists(os.path.join(SRC_DIR, 'main.cpp')):
    with open(os.path.join(SRC_DIR, 'main.cpp'), 'r', encoding='utf-8') as f:
        content = f.read()
    with open(os.path.join(SRC_DIR, 'Stage', 'StageTest_main.cpp'), 'w', encoding='utf-8') as f:
        f.write('#if 0 // Conflict disabled\n' + content + '\n#endif\n')
    os.remove(os.path.join(SRC_DIR, 'main.cpp'))

# Fix includes in Stage files
for sf in stage_files:
    path = os.path.join(SRC_DIR, 'Stage', sf)
    if os.path.exists(path):
        with open(path, 'r', encoding='utf-8') as f:
            code = f.read()
        code = code.replace('#include "Object2D.h"', '#include "StageTypes.h"')
        with open(path, 'w', encoding='utf-8') as f:
            f.write(code)

# 2. Rescue our main.cpp and Object2D.h from outer Source
if os.path.exists(os.path.join(OUTER_SRC, 'main.cpp')):
    shutil.move(os.path.join(OUTER_SRC, 'main.cpp'), os.path.join(SRC_DIR, 'Core', 'main.cpp'))
if os.path.exists(os.path.join(OUTER_SRC, 'Object2D.h')):
    shutil.move(os.path.join(OUTER_SRC, 'Object2D.h'), os.path.join(SRC_DIR, 'Objects', 'Object2D.h'))

# 3. Categorize and move all root files
file_map = {
    'Vector2.h': 'Core',
    'ObjectManager.h': 'Managers', 'ObjectManager.cpp': 'Managers',
    'ColliderManager.h': 'Managers', 'ColliderManager.cpp': 'Managers',
    'SceneManager.h': 'Managers', 'SceneManager.cpp': 'Managers',
    'SoundManager.h': 'Managers', 'SoundManager.cpp': 'Managers',
    'EffectManager.h': 'Managers', 'EffectManager.cpp': 'Managers',
    'InputManager.h': 'Managers', 'InputManager.cpp': 'Managers',
    'Scene.h': 'Scenes', 'Scene.cpp': 'Scenes',
    'TitleScene.h': 'Scenes', 'TitleScene.cpp': 'Scenes',
    'GameScene.h': 'Scenes', 'GameScene.cpp': 'Scenes',
    'ResultScene.h': 'Scenes', 'ResultScene.cpp': 'Scenes',
    'Object2D.cpp': 'Objects',
    'Character.h': 'Characters', 'Character.cpp': 'Characters',
    'Player.h': 'Characters', 'Player.cpp': 'Characters',
    'Enemy.h': 'Characters', 'Enemy.cpp': 'Characters',
    'Weapon.h': 'Weapons', 'Weapon.cpp': 'Weapons',
    'Handgun.h': 'Weapons', 'Handgun.cpp': 'Weapons',
    'Shotgun.h': 'Weapons', 'Shotgun.cpp': 'Weapons',
    'Bullet.h': 'Weapons', 'Bullet.cpp': 'Weapons',
    'Collider.h': 'Colliders', 'Collider.cpp': 'Colliders',
    'CircleCollider.h': 'Colliders', 'CircleCollider.cpp': 'Colliders',
    'RectCollider.h': 'Colliders', 'RectCollider.cpp': 'Colliders'
}

for root_file, folder in file_map.items():
    p = os.path.join(PROJ_ROOT, root_file)
    if os.path.exists(p):
        shutil.move(p, os.path.join(SRC_DIR, folder, root_file))

# 4. Generate new .vcxproj and .vcxproj.filters
proj = os.path.join(PROJ_ROOT, 'TeamGame.vcxproj')
filters = os.path.join(PROJ_ROOT, 'TeamGame.vcxproj.filters')

ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}
tree = ET.parse(proj)
root = tree.getroot()

# Remove all existing ItemGroups that contain ClCompile or ClInclude
for ig in root.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None or ig.find('ms:ClInclude', ns) is not None:
        root.remove(ig)

# Add Include Directories for every folder in Source so includes like #include "Player.h" still work!
for idg in root.findall('ms:ItemDefinitionGroup', ns):
    cl = idg.find('ms:ClCompile', ns)
    if cl is not None:
        inc = cl.find('ms:AdditionalIncludeDirectories', ns)
        if inc is not None:
            inc.text = '..\\DxLib_VC\\プロジェクトに追加すべきファイル_VC用;Source;' + ';'.join(['Source\\' + d for d in dirs]) + ';%(AdditionalIncludeDirectories)'

# Collect all files
all_cpps = []
all_hs = []
for d in dirs:
    dpath = os.path.join(SRC_DIR, d)
    for f in os.listdir(dpath):
        if f.endswith('.cpp'):
            all_cpps.append((os.path.join('Source', d, f), d))
        elif f.endswith('.h'):
            all_hs.append((os.path.join('Source', d, f), d))

# Add them to vcxproj
ig_cpp = ET.SubElement(root, 'ItemGroup')
for f, _ in all_cpps:
    cl = ET.SubElement(ig_cpp, 'ClCompile')
    cl.set('Include', f)
    # add utf-8 flag
    opt = ET.SubElement(cl, 'AdditionalOptions')
    opt.text = '/utf-8 /wd4828 /wd4010 %(AdditionalOptions)'

ig_h = ET.SubElement(root, 'ItemGroup')
for f, _ in all_hs:
    cl = ET.SubElement(ig_h, 'ClInclude')
    cl.set('Include', f)

tree.write(proj, encoding='utf-8', xml_declaration=True)

# Generate filters file
tree_f = ET.parse(filters)
root_f = tree_f.getroot()
for ig in root_f.findall('ms:ItemGroup', ns):
    root_f.remove(ig)

# Filter definitions
ig_def = ET.SubElement(root_f, 'ItemGroup')
for d in dirs:
    filt = ET.SubElement(ig_def, 'Filter')
    filt.set('Include', 'Source Files\\' + d)
    filt = ET.SubElement(ig_def, 'Filter')
    filt.set('Include', 'Header Files\\' + d)

ig_items = ET.SubElement(root_f, 'ItemGroup')
for f, d in all_cpps:
    cl = ET.SubElement(ig_items, 'ClCompile')
    cl.set('Include', f)
    filt = ET.SubElement(cl, 'Filter')
    filt.text = 'Source Files\\' + d

for f, d in all_hs:
    cl = ET.SubElement(ig_items, 'ClInclude')
    cl.set('Include', f)
    filt = ET.SubElement(cl, 'Filter')
    filt.text = 'Header Files\\' + d

tree_f.write(filters, encoding='utf-8', xml_declaration=True)

print("Project restructuring completed.")
