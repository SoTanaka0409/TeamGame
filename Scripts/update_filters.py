import os
import xml.etree.ElementTree as ET

filters_path = r'C:\Users\student\Desktop\TeamGame\TeamGame\TeamGame\TeamGame.vcxproj.filters'
ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')
tree = ET.parse(filters_path)
root = tree.getroot()
ns = {'ms': 'http://schemas.microsoft.com/developer/msbuild/2003'}

# Get existing nodes or create them
compile_group = None
include_group = None

for ig in root.findall('ms:ItemGroup', ns):
    if ig.find('ms:ClCompile', ns) is not None:
        compile_group = ig
    elif ig.find('ms:ClInclude', ns) is not None:
        include_group = ig

if compile_group is None:
    compile_group = ET.SubElement(root, 'ItemGroup')
if include_group is None:
    include_group = ET.SubElement(root, 'ItemGroup')

cpp_files = ['main.cpp', 'Object2D.cpp', 'ObjectManager.cpp', 'Collider.cpp', 'ColliderManager.cpp', 'SceneManager.cpp', 'SoundManager.cpp', 'EffectManager.cpp', 'Player.cpp']
h_files = ['Object2D.h', 'ObjectManager.h', 'Collider.h', 'ColliderManager.h', 'BaseScene.h', 'SceneManager.h', 'SoundManager.h', 'EffectManager.h', 'Player.h']

existing_cpps = [c.get('Include') for c in compile_group.findall('ms:ClCompile', ns)]
existing_hs = [c.get('Include') for c in include_group.findall('ms:ClInclude', ns)]

for f in cpp_files:
    if f not in existing_cpps:
        el = ET.SubElement(compile_group, 'ClCompile')
        el.set('Include', f)
        filt = ET.SubElement(el, 'Filter')
        filt.text = 'ソース ファイル'

for f in h_files:
    if f not in existing_hs:
        el = ET.SubElement(include_group, 'ClInclude')
        el.set('Include', f)
        filt = ET.SubElement(el, 'Filter')
        filt.text = 'ヘッダー ファイル'

tree.write(filters_path, encoding='utf-8', xml_declaration=True)
