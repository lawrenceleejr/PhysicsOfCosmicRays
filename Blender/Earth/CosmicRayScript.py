import bpy
import csv
from mathutils import Vector

PYTHIA_DATA_FOLDER = "/Users/devlin/Desktop/Blender/Earth/Pythia"
EVENT_NUMBER = 1
SCALE = 0.001
TRACK_LENGTH_SCALE = 1
BEAM_FRAMES = 30
ANIMATION_SPEED = 0.01
ORIGIN_OFFSET = Vector((0, 0, 2))

def get_color_from_time(time_fraction):
    if time_fraction < 0.2:
        t = time_fraction / 0.2
        return (0.0, t * 0.5, 1.0)
    elif time_fraction < 0.4:
        t = (time_fraction - 0.2) / 0.2
        return (0.0, 0.5 + t * 0.5, 1.0 - t)
    elif time_fraction < 0.6:
        t = (time_fraction - 0.4) / 0.2
        return (t, 1.0, 0.0)
    elif time_fraction < 0.8:
        t = (time_fraction - 0.6) / 0.2
        return (1.0, 1.0 - t * 0.3, 0.0)
    else:
        t = (time_fraction - 0.8) / 0.2
        return (1.0, 0.7 - t * 0.7, 0.0)

import os
filepath = os.path.join(PYTHIA_DATA_FOLDER, f"event_{EVENT_NUMBER:04d}.csv")

collection = bpy.data.collections.new("CosmicRay")
bpy.context.scene.collection.children.link(collection)

beam_curve = bpy.data.curves.new("IncomingRay", 'CURVE')
beam_curve.dimensions = '3D'
beam_curve.bevel_depth = 0.01
beam_curve.bevel_resolution = 4

spline = beam_curve.splines.new('POLY')
spline.points.add(1)
spline.points[0].co = (*(Vector((0, 0, 50)) + ORIGIN_OFFSET), 1)
spline.points[1].co = (*ORIGIN_OFFSET, 1)

beam = bpy.data.objects.new("IncomingRay", beam_curve)
collection.objects.link(beam)

beam_mat = bpy.data.materials.new("BeamMat")
beam_mat.use_nodes = True
nodes = beam_mat.node_tree.nodes
nodes.clear()
emission = nodes.new('ShaderNodeEmission')
output = nodes.new('ShaderNodeOutputMaterial')
emission.inputs['Color'].default_value = (1.0, 0.2, 0.2, 1.0)
emission.inputs['Strength'].default_value = 5.0
beam_mat.node_tree.links.new(emission.outputs[0], output.inputs[0])
beam.data.materials.append(beam_mat)

beam.hide_viewport = True
beam.hide_render = True
beam.keyframe_insert(data_path="hide_viewport", frame=1)
beam.keyframe_insert(data_path="hide_render", frame=1)
beam.hide_viewport = False
beam.hide_render = False
beam.keyframe_insert(data_path="hide_viewport", frame=BEAM_FRAMES)
beam.keyframe_insert(data_path="hide_render", frame=BEAM_FRAMES)

tracks = []
with open(filepath, 'r') as f:
    reader = csv.DictReader(f)
    for row in reader:
        try:
            start = Vector((
                float(row['x_start']) * SCALE,
                float(row['y_start']) * SCALE,
                float(row['z_start']) * SCALE
            )) + ORIGIN_OFFSET
            end_orig = Vector((
                float(row['x_end']) * SCALE,
                float(row['y_end']) * SCALE,
                float(row['z_end']) * SCALE
            )) + ORIGIN_OFFSET
            
            direction = end_orig - start
            end = start + direction * TRACK_LENGTH_SCALE
            
            if direction.length > 0.01 and direction.z <= 0:
                t_start = float(row['t_start'])
                t_end = float(row['t_end'])
                t_avg = (t_start + t_end) / 2.0
                tracks.append({
                    'start': start,
                    'end': end,
                    't_start': t_start,
                    't_avg': t_avg
                })
        except:
            continue

times = [t['t_avg'] for t in tracks if t['t_avg'] > 0]
min_time = min(times) if times else 0
max_time = max(times) if times else 1
time_range = max_time - min_time if max_time != min_time else 1

for track in tracks:
    if track['t_avg'] == 0:
        track['time_frac'] = 0.0
    else:
        track['time_frac'] = (track['t_avg'] - min_time) / time_range
    track['color'] = get_color_from_time(track['time_frac'])

tracks.sort(key=lambda t: t['t_start'])

for i, track in enumerate(tracks):
    curve = bpy.data.curves.new(f"Track_{i}", 'CURVE')
    curve.dimensions = '3D'
    curve.bevel_depth = 0.001
    curve.bevel_resolution = 2
    
    spline = curve.splines.new('POLY')
    spline.points.add(1)
    spline.points[0].co = (*track['start'], 1)
    spline.points[1].co = (*track['end'], 1)
    
    obj = bpy.data.objects.new(f"Track_{i}", curve)
    collection.objects.link(obj)
    
    mat = bpy.data.materials.new(f"Mat_{i}")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    nodes.clear()
    
    emission = nodes.new('ShaderNodeEmission')
    output = nodes.new('ShaderNodeOutputMaterial')
    emission.inputs['Color'].default_value = (*track['color'], 1.0)
    emission.inputs['Strength'].default_value = 2.5
    
    mat.node_tree.links.new(emission.outputs[0], output.inputs[0])
    obj.data.materials.append(mat)
    
    appear_frame = BEAM_FRAMES + int(track['t_start'] * ANIMATION_SPEED)
    
    obj.hide_viewport = True
    obj.hide_render = True
    obj.keyframe_insert(data_path="hide_viewport", frame=appear_frame - 1)
    obj.keyframe_insert(data_path="hide_render", frame=appear_frame - 1)
    
    obj.hide_viewport = False
    obj.hide_render = False
    obj.keyframe_insert(data_path="hide_viewport", frame=appear_frame)
    obj.keyframe_insert(data_path="hide_render", frame=appear_frame)

last_frame = BEAM_FRAMES + int(max([t['t_start'] for t in tracks]) * ANIMATION_SPEED) + 50
bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = last_frame

print(f"Animation complete: {len(tracks)} tracks")
print(f"Track length scale: {TRACK_LENGTH_SCALE}x")
print(f"Total frames: {last_frame}")