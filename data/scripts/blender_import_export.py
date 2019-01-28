import bpy

print("Hello World")

filepath = '/home/hoqe4365/Desktop/lamure/lamure/install/bin/data/'
filename = 'bunny_all.obj'
f = filepath + filename
outfilename = f
#bpy.ops.wm.read_factory_settings(use_empty=True)

bpy.ops.import_scene.obj(filepath=f)
bpy.ops.export_scene.obj(filepath=outfilename,use_materials=False, use_triangles=True,use_normals=True,use_uvs=True)
