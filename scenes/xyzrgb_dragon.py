#!/usr/bin/env python

# 1 xyzrgb_dragon with 32 point lights
# Copyright (c) 2011-2016 Hiroshi Tsubokawa

import fujiyama
si = fujiyama.SceneInterface()

#plugins
si.OpenPlugin('PlasticShader')
si.OpenPlugin('GlassShader')
si.OpenPlugin('ConstantShader')

#Camera
si.NewCamera('cam1', 'PerspectiveCamera')
si.SetProperty3('cam1', 'translate', 0, 1.5, 7)
si.SetProperty3('cam1', 'rotate', -5.7105931375, 0, 0)

#Light
si.NewLight('light0', 'PointLight')
si.SetProperty3('light0', 'translate', 0.900771, 12, 4.09137)
si.SetProperty1('light0', 'intensity', 0.03125)
si.NewLight('light1', 'PointLight')
si.SetProperty3('light1', 'translate', 2.02315, 12, 5.28021)
si.SetProperty1('light1', 'intensity', 0.03125)
si.NewLight('light2', 'PointLight')
si.SetProperty3('light2', 'translate', 10.69, 12, 13.918)
si.SetProperty1('light2', 'intensity', 0.03125)
si.NewLight('light3', 'PointLight')
si.SetProperty3('light3', 'translate', 4.28027, 12, 7.58462)
si.SetProperty1('light3', 'intensity', 0.03125)
si.NewLight('light4', 'PointLight')
si.SetProperty3('light4', 'translate', 12.9548, 12, 1.19914)
si.SetProperty1('light4', 'intensity', 0.03125)
si.NewLight('light5', 'PointLight')
si.SetProperty3('light5', 'translate', 6.55808, 12, 2.31772)
si.SetProperty1('light5', 'intensity', 0.03125)
si.NewLight('light6', 'PointLight')
si.SetProperty3('light6', 'translate', 0.169064, 12, 10.9623)
si.SetProperty1('light6', 'intensity', 0.03125)
si.NewLight('light7', 'PointLight')
si.SetProperty3('light7', 'translate', 1.25002, 12, 4.51314)
si.SetProperty1('light7', 'intensity', 0.03125)
si.NewLight('light8', 'PointLight')
si.SetProperty3('light8', 'translate', 2.46758, 12, 5.73382)
si.SetProperty1('light8', 'intensity', 0.03125)
si.NewLight('light9', 'PointLight')
si.SetProperty3('light9', 'translate', 3.55644, 12, 6.84334)
si.SetProperty1('light9', 'intensity', 0.03125)
si.NewLight('light10', 'PointLight')
si.SetProperty3('light10', 'translate', 4.76112, 12, 8.00264)
si.SetProperty1('light10', 'intensity', 0.03125)
si.NewLight('light11', 'PointLight')
si.SetProperty3('light11', 'translate', 13.3267, 12, 9.10333)
si.SetProperty1('light11', 'intensity', 0.03125)
si.NewLight('light12', 'PointLight')
si.SetProperty3('light12', 'translate', 14.4155, 12, 2.68084)
si.SetProperty1('light12', 'intensity', 0.03125)
si.NewLight('light13', 'PointLight')
si.SetProperty3('light13', 'translate', 8.10755, 12, 3.79629)
si.SetProperty1('light13', 'intensity', 0.03125)
si.NewLight('light14', 'PointLight')
si.SetProperty3('light14', 'translate', 9.21103, 12, 4.9484)
si.SetProperty1('light14', 'intensity', 0.03125)
si.NewLight('light15', 'PointLight')
si.SetProperty3('light15', 'translate', 2.83469, 12, 6.09221)
si.SetProperty1('light15', 'intensity', 0.03125)
si.NewLight('light16', 'PointLight')
si.SetProperty3('light16', 'translate', 4.00945, 12, 7.18302)
si.SetProperty1('light16', 'intensity', 0.03125)
si.NewLight('light17', 'PointLight')
si.SetProperty3('light17', 'translate', 12.6072, 12, 0.832089)
si.SetProperty1('light17', 'intensity', 0.03125)
si.NewLight('light18', 'PointLight')
si.SetProperty3('light18', 'translate', 6.21169, 12, 1.98055)
si.SetProperty1('light18', 'intensity', 0.03125)
si.NewLight('light19', 'PointLight')
si.SetProperty3('light19', 'translate', 7.39599, 12, 10.5563)
si.SetProperty1('light19', 'intensity', 0.03125)
si.NewLight('light20', 'PointLight')
si.SetProperty3('light20', 'translate', 8.52421, 12, 4.15086)
si.SetProperty1('light20', 'intensity', 0.03125)
si.NewLight('light21', 'PointLight')
si.SetProperty3('light21', 'translate', 9.5891, 12, 5.39715)
si.SetProperty1('light21', 'intensity', 0.03125)
si.NewLight('light22', 'PointLight')
si.SetProperty3('light22', 'translate', 3.18967, 12, 13.9542)
si.SetProperty1('light22', 'intensity', 0.03125)
si.NewLight('light23', 'PointLight')
si.SetProperty3('light23', 'translate', 4.41432, 12, 0.082813)
si.SetProperty1('light23', 'intensity', 0.03125)
si.NewLight('light24', 'PointLight')
si.SetProperty3('light24', 'translate', 5.48803, 12, 1.21856)
si.SetProperty1('light24', 'intensity', 0.03125)
si.NewLight('light25', 'PointLight')
si.SetProperty3('light25', 'translate', 6.57647, 12, 2.31432)
si.SetProperty1('light25', 'intensity', 0.03125)
si.NewLight('light26', 'PointLight')
si.SetProperty3('light26', 'translate', 0.265098, 12, 10.9453)
si.SetProperty1('light26', 'intensity', 0.03125)
si.NewLight('light27', 'PointLight')
si.SetProperty3('light27', 'translate', 8.84422, 12, 12.1117)
si.SetProperty1('light27', 'intensity', 0.03125)
si.NewLight('light28', 'PointLight')
si.SetProperty3('light28', 'translate', 10.0154, 12, 5.67625)
si.SetProperty1('light28', 'intensity', 0.03125)
si.NewLight('light29', 'PointLight')
si.SetProperty3('light29', 'translate', 11.0907, 12, 14.4043)
si.SetProperty1('light29', 'intensity', 0.03125)
si.NewLight('light30', 'PointLight')
si.SetProperty3('light30', 'translate', 4.71726, 12, 7.98851)
si.SetProperty1('light30', 'intensity', 0.03125)
si.NewLight('light31', 'PointLight')
si.SetProperty3('light31', 'translate', 13.3907, 12, 9.08986)
si.SetProperty1('light31', 'intensity', 0.03125)

#Texture
si.NewTexture('tex1', '../../hdr/pisa.hdr')

#Shader
si.NewShader('xyzrgb_dragon_shader0', 'PlasticShader')
si.SetProperty1('xyzrgb_dragon_shader0', 'ior', 50)
si.SetProperty3('xyzrgb_dragon_shader0', 'diffuse', 0, 0, 0)
si.NewShader('floor_shader', 'PlasticShader')
si.NewShader('dome_shader', 'ConstantShader')

#Mesh
si.NewMesh('xyzrgb_dragon_mesh', '../../ply/xyzrgb_dragon.ply')
si.NewMesh('floor_mesh', '../../ply/floor.ply')
si.NewMesh('dome_mesh', '../../ply/dome.ply')

#ObjectInstance
si.NewObjectInstance('xyzrgb_dragon1', 'xyzrgb_dragon_mesh')
si.SetProperty3('xyzrgb_dragon1', 'scale', 0.5, 0.5, 0.5)
si.SetProperty3('xyzrgb_dragon1', 'rotate', 0, -35.0, 0)
si.SetProperty3('xyzrgb_dragon1', 'translate', 0.2, 0, 0)
si.AssignShader('xyzrgb_dragon1', 'DEFAULT_SHADING_GROUP', 'xyzrgb_dragon_shader0')

si.NewObjectInstance('floor1', 'floor_mesh')
si.AssignShader('floor1', 'DEFAULT_SHADING_GROUP', 'floor_shader')

si.NewObjectInstance('dome1', 'dome_mesh')
si.SetProperty3('dome1', 'scale', 0.5, 0.5, 0.5)
si.SetProperty3('dome1', 'rotate', 0, 180, 0)
si.AssignShader('dome1', 'DEFAULT_SHADING_GROUP', 'dome_shader')
si.AssignTexture('dome_shader', 'texture', 'tex1')

#ObjectGroup
si.NewObjectGroup('group1')
si.AddObjectToGroup('group1', 'xyzrgb_dragon1')
si.AssignObjectGroup('xyzrgb_dragon1', 'shadow_target', 'group1')
si.AssignObjectGroup('floor1', 'shadow_target', 'group1')

#FrameBuffer
si.NewFrameBuffer('fb1', 'rgba')

#Renderer
si.NewRenderer('ren1')
si.AssignCamera('ren1', 'cam1')
si.AssignFrameBuffer('ren1', 'fb1')
si.SetProperty2('ren1', 'resolution', 640, 480)
#si.SetProperty2('ren1', 'resolution', 160, 120)

#Rendering
si.RenderScene('ren1')

#Output
si.SaveFrameBuffer('fb1', '../xyzrgb_dragon.fb')

#Run commands
si.Run()
#si.Print()
