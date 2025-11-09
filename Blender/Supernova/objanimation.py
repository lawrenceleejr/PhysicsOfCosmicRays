import bpy
import os
import re

# === USER SETTINGS ===
folder = "/Users/leejr/Dropbox/Professional/Tennessee/Teaching/2526_Fall_SpecialTopics/PhysicsOfCosmicRays/Blender/Supernova/ni60_obj"
pattern = re.compile(r"_(\d+)")   # capture numbers after underscore
collection_name = "OBJ_Sequence"
display_object_name = "AnimatedOBJ"
scale_factor = 5e-14
frame_step = 3       # use every Nth OBJ
max_frames = 70      # truncate after this many frames, None = all
material_name = "SupernovaMat"

# === CREATE MATERIAL ===
if material_name in bpy.data.materials:
    mat = bpy.data.materials[material_name]
else:
    mat = bpy.data.materials.new(material_name)
    mat.use_nodes = True

# === PREPARE COLLECTION ===
if collection_name in bpy.data.collections:
    bpy.data.collections.remove(bpy.data.collections[collection_name])
coll = bpy.data.collections.new(collection_name)
bpy.context.scene.collection.children.link(coll)

# === FIND AND SORT OBJs ===
files = [f for f in os.listdir(folder) if f.lower().endswith(".obj")]
files.sort(key=lambda f: int(pattern.search(f).group(1)) if pattern.search(f) else f)

# Apply frame skipping
files = files[::frame_step]

# Apply max_frames truncation
if max_frames is not None:
    files = files[:max_frames]

print(f"Using {len(files)} OBJs after frame_step={frame_step} and max_frames={max_frames}")

# === IMPORT MESHES INTO HIDDEN COLLECTION ===
imported_meshes = []

for i, fname in enumerate(files):
    path = os.path.join(folder, fname)
    bpy.ops.wm.obj_import(filepath=path)
    imported_objs = [obj for obj in bpy.context.selected_objects if obj.type == 'MESH']
    if not imported_objs:
        continue
    obj = imported_objs[0]

    # Move to hidden collection
    for c in obj.users_collection:
        c.objects.unlink(obj)
    coll.objects.link(obj)
    obj.hide_viewport = True
    obj.hide_render = True

    # Clear any materials to allow a consistent one
    obj.data.materials.clear()

    imported_meshes.append(obj.data)

    # Remove object, keep mesh
    bpy.data.objects.remove(obj, do_unlink=True)

print(f"✅ Imported {len(imported_meshes)} meshes.")

# === CREATE DISPLAY OBJECT ===
if display_object_name in bpy.data.objects:
    bpy.data.objects.remove(bpy.data.objects[display_object_name], do_unlink=True)

display_obj = bpy.data.objects.new(display_object_name, imported_meshes[0])
display_obj.scale = (scale_factor, scale_factor, scale_factor)
bpy.context.scene.collection.objects.link(display_obj)

# Assign the material
display_obj.data.materials.clear()
display_obj.data.materials.append(mat)

# === FRAME HANDLER TO SWAP MESHES AND APPLY MATERIAL ===
def update_obj_sequence(scene):
    frame = scene.frame_current
    index = (frame - scene.frame_start) % len(imported_meshes)
    display_obj.data = imported_meshes[index]

    # Ensure the material is always applied
    if not display_obj.data.materials:
        display_obj.data.materials.append(mat)
    else:
        display_obj.data.materials[0] = mat

# Remove any previous handlers
bpy.app.handlers.frame_change_pre.clear()
bpy.app.handlers.frame_change_pre.append(update_obj_sequence)

# === SET FRAME RANGE ===
scene = bpy.context.scene
scene.frame_start = 1
scene.frame_end = len(imported_meshes)

print("🎬 Ready! Single object animation setup with consistent material.")