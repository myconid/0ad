import bpy
import os
import math
from mathutils import Matrix

def near0(a):
    if abs(a) < 0.00000001:
        return True
    else:
        return False


class AOBaker(bpy.types.Operator):

    bl_idname = "object.ao_baker"
    bl_label = "AO Baker"
    
    def execute(self, context):

        srcdir = ""
        texdestdir = ""
        modeldestdir = ""
        
        handedness = not True

        # create result image, 1024x1024
        bpy.ops.image.new(name="lightmap")
        
        # iterate through files in srcdir
        for filename in os.listdir(srcdir):
        
            try:
                # clear meshes if they exist
                for object_name in bpy.data.objects:
                    object_name.select = True
                bpy.ops.object.delete()
                for item in bpy.data.meshes:
                    bpy.data.meshes.remove(item)
            
                # for each Collada model
                if filename.lower().endswith(".dae"):

                    print ("** BAKING: " + filename)
                
                    # import it
                    bpy.ops.wm.collada_import(filepath=srcdir + "/" + filename)
                
                    obj = None
                    for o in bpy.data.objects:
                        if o.type == 'MESH':
                            obj = o
                            break
                
                    # change coordinate system handedness
                    if handedness:
                        obj.location[1], obj.location[2] = -obj.location[2], obj.location[1]
                        mat = Matrix.Rotation(math.pi / 2.0, 4, 'X')
                        obj.matrix_world = mat * obj.matrix_world

                    # add UV set and select it
                    obj.data.uv_textures.new("lightmap")
                    obj.data.uv_textures.active = obj.data.uv_textures[1]
                
                    # select model, go into edit mode, select all vertices
                    # unwrap with smart project, exit edit mode
                    bpy.context.scene.objects.active = obj
                    bpy.ops.object.editmode_toggle()
                    bpy.ops.mesh.select_all()
                    bpy.ops.uv.smart_project()
                    bpy.ops.object.editmode_toggle()
            
                    # set the lightmap texture on all model faces
                    for f in obj.data.uv_textures.active.data[:]:
                        f.image = bpy.data.images["lightmap"]
                
                    # bake AO
                    bpy.context.scene.render.bake_type = "AO"
                    bpy.ops.object.bake_image()

                    # write image and mesh to destination dirs
                    bname = os.path.splitext(os.path.basename(filename))[0]
                    bpy.data.images["lightmap"].save_render(filepath=texdestdir + "/" + bname + ".png")
                    bpy.ops.wm.collada_export(filepath=modeldestdir + "/" + bname + ".dae")
            except:
                print ("****** FAILED ******")
        return {'FINISHED'}

def register():
    bpy.utils.register_class(AOBaker)


def unregister():
    bpy.utils.unregister_class(AOBaker)

if __name__ == "__main__":
    register()
    bpy.ops.object.ao_baker()