import bpy

print("Hello World")

filepath = '/home/hoqe4365/Desktop/lamure/lamure/install/bin/data/'
filename = 'lion_all.obj'
f = filepath + filename
of = f
outfilepath = '/home/hoqe4365/Desktop/lamure/lamure/install/bin/data/'
outfilename = 'lion_all.obj'
of = outfilepath + outfilename

#delete all meshes in scene
for ob in bpy.context.scene.objects:
    ob.select = ob.type == 'MESH'
bpy.ops.object.delete()

#import and export
bpy.ops.import_scene.obj(filepath=f)
bpy.ops.export_scene.obj(filepath=of,use_materials=False, use_triangles=True,use_normals=True,use_uvs=True)



