import bpy
import csv
from mathutils import Vector
from math import sin, cos, radians
import numpy as np
import random
import os
import glob

PYTHIA_DATA_FOLDER = "/Users/devlin/Desktop/PhysicsOfCosmicRays/Blender/Earth/Pythia"

# Spherical to Cartesian
def sph_vec(r, theta_deg, phi_deg):
    th, ph = radians(theta_deg), radians(phi_deg)
    return Vector((
        r * sin(ph) * cos(th),
        r * sin(ph) * sin(th),
        r * cos(ph)
    ))

# ranges
R_RANGE     = (1.1, 1.05)   # ~1 is atmosphere
THETA_RANGE = (0.0, 180.0) # 0-360 
PHI_RANGE   = (15, 65) # 0-180

def rand_origin(r_rng, th_rng, phi_rng):
    r  = np.random.uniform(*r_rng)
    th = np.random.uniform(*th_rng)
    ph = np.random.uniform(*phi_rng)
    return sph_vec(r, th, ph)

ORIGIN_OFFSET = rand_origin(R_RANGE, THETA_RANGE, PHI_RANGE)

# Get a random CSV file from the folder
csv_files = glob.glob(os.path.join(PYTHIA_DATA_FOLDER, "event_*.csv"))
if not csv_files:
    raise FileNotFoundError(f"No CSV files found in {PYTHIA_DATA_FOLDER}")
filepath = random.choice(csv_files)
print(f"Loading: {filepath}")

SCALE = 0.001
TRACK_LENGTH_SCALE = 1
BEAM_FRAMES = 30
ANIMATION_SPEED = 0.01

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

collection = bpy.data.collections.new("CosmicRay")
bpy.context.scene.collection.children.link(collection)

beam_curve = bpy.data.curves.new("IncomingRay", 'CURVE')
beam_curve.dimensions = '3D'
beam_curve.bevel_depth = 0.01
beam_curve.bevel_resolution = 4

spline = beam_curve.splines.new('POLY')
spline.points.add(1)
FAR = 50.0 # how far away the incoming cosmic ray goes out
to_origin = (-ORIGIN_OFFSET).normalized() # points to origin 0,0,0
spline.points[0].co = (*(ORIGIN_OFFSET - to_origin * FAR), 1)
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
emission.inputs['Strength'].default_value = 100
beam_mat.node_tree.links.new(emission.outputs[0], output.inputs[0])
beam.data.materials.append(beam_mat)

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