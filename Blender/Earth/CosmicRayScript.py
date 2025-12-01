import bpy
import csv
from mathutils import Vector
from math import sin, cos, radians
import numpy as np
import random
import os
import glob
import math


PYTHIA_DATA_FOLDER = "C:/Users/Administrator/Desktop/Blender/Earth/Pythia"

# Global timing
FPS = 24
SCENE_SECONDS = 120
EVENT_GAP_SEC = 1.0
SCENE_FRAMES = int(SCENE_SECONDS * FPS)
EVENT_GAP_FRAMES = int(EVENT_GAP_SEC * FPS)

# Per-event timing
RAY_TIP_SPEED = 0.05
BEAM_GROW_FRAMES = 10.0
HOLD_AFTER_FULL_SEC = 0.5
FADE_DURATION_FRAMES = 42
HOLD_FRAMES = int(max(0, HOLD_AFTER_FULL_SEC * FPS))

# Geometry params
SCALE = 0.001
TRACK_LENGTH_SCALE = 1.0
R_RANGE = (1.05, 1.10)
THETA_RANGE = (240, 60)
PHI_RANGE = (25, 155)
COLLISION_TOLERANCE = 0.01
FAR_DISTANCE = 15.0
TARGET_SPHERE_RADIUS = 0.5

# Emission brightness (reduced for subtler effect)
OVERSHOULDER_BEAM_BRIGHTNESS = 60.0  # Was 150.0
NORMAL_BEAM_BRIGHTNESS = 40.0        # Was 100.0
TRACK_BRIGHTNESS = 20.0              # Was 50.0

# Camera position for "over the shoulder" shot
CAMERA_POS = Vector((-3.72767, -10.6855, 2.24198))

def sph_vec(r, theta_deg, phi_deg):
    th, ph = radians(theta_deg), radians(phi_deg)
    return Vector((r * sin(ph) * cos(th),
                   r * sin(ph) * sin(th),
                   r * cos(ph)))

def random_point_in_sphere(radius):
    """Generate a random point uniformly distributed within a sphere"""
    u = np.random.uniform(0, 1)
    r = radius * (u ** (1/3))
    
    theta = np.random.uniform(0, 2 * np.pi)
    phi = np.arccos(np.random.uniform(-1, 1))
    
    x = r * np.sin(phi) * np.cos(theta)
    y = r * np.sin(phi) * np.sin(theta)
    z = r * np.cos(phi)
    
    return Vector((x, y, z))

def rand_origin(r_rng, th_rng, phi_rng):
    r  = np.random.uniform(*r_rng)
    
    th_start, th_end = th_rng
    if th_start > th_end:
        if np.random.rand() < (360 - th_start) / ((360 - th_start) + th_end):
            th = np.random.uniform(th_start, 360)
        else:
            th = np.random.uniform(0, th_end)
    else:
        th = np.random.uniform(th_start, th_end)
    
    ph = np.random.uniform(*phi_rng)
    return sph_vec(r, th, ph)

def set_linear_fcurves(id_block):
    if getattr(id_block, "animation_data", None) and id_block.animation_data.action:
        for fc in id_block.animation_data.action.fcurves:
            for kp in fc.keyframe_points:
                kp.interpolation = 'LINEAR'

def set_smooth_emission_fade(material):
    """Set smooth bezier interpolation for emission strength fade-out"""
    if not material.node_tree or not material.node_tree.animation_data:
        return
    
    action = material.node_tree.animation_data.action
    if not action:
        return
    
    # Find the emission strength fcurve
    for fc in action.fcurves:
        if 'Strength' in fc.data_path and 'default_value' in fc.data_path:
            for kp in fc.keyframe_points:
                kp.interpolation = 'BEZIER'
                kp.handle_left_type = 'AUTO_CLAMPED'
                kp.handle_right_type = 'AUTO_CLAMPED'

def ensure_collection(name):
    col = bpy.data.collections.get(name)
    if not col:
        col = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(col)
    return col

# Scene Setup
scene = bpy.context.scene
scene.render.fps = FPS
scene.frame_start = 1
scene.frame_end = SCENE_FRAMES
scene.frame_current = 1

master_col = ensure_collection("CosmicRay_Looped")

earth_obj = bpy.data.objects.get("Earth")
if not earth_obj:
    print("WARNING: No Earth object found.")

# CSVs
csv_files = glob.glob(os.path.join(PYTHIA_DATA_FOLDER, "event_*.csv"))
if not csv_files:
    raise FileNotFoundError(f"No CSV files found in {PYTHIA_DATA_FOLDER}")

# Special "over the shoulder" event builder
def build_over_shoulder_event(start_frame: int, evt_idx: int) -> int:
    """Creates a cosmic ray that passes VERY close to the camera for dramatic effect"""
    evt_col = bpy.data.collections.new(f"CosmicRay_OverShoulder_{evt_idx:02d}")
    master_col.children.link(evt_col)
    
    filepath = random.choice(csv_files)
    print(f"[Event {evt_idx} - OVER SHOULDER] CSV: {os.path.basename(filepath)}")
    
    camera_dir = CAMERA_POS.normalized()
    r_impact = np.random.uniform(*R_RANGE)
    
    theta_offset = np.random.uniform(-0.05, 0.05)
    phi_offset = np.random.uniform(-0.05, 0.05)
    
    cam_r = CAMERA_POS.length
    cam_theta = math.degrees(math.atan2(CAMERA_POS.y, CAMERA_POS.x))
    cam_phi = math.degrees(math.acos(CAMERA_POS.z / cam_r))
    
    ORIGIN_OFFSET = sph_vec(r_impact, cam_theta + theta_offset, cam_phi + phi_offset)
    
    target_point = random_point_in_sphere(TARGET_SPHERE_RADIUS)
    to_target = (target_point - ORIGIN_OFFSET).normalized()
    
    camera_to_origin_dist = CAMERA_POS.length
    start_dist = camera_to_origin_dist + 15.0
    far_pt = ORIGIN_OFFSET - to_target * start_dist
    
    ray_dir = (ORIGIN_OFFSET - far_pt).normalized()
    cam_to_ray_start = CAMERA_POS - far_pt
    closest_point_param = cam_to_ray_start.dot(ray_dir)
    closest_point = far_pt + ray_dir * closest_point_param
    distance_to_camera = (CAMERA_POS - closest_point).length
    
    pythia_down = Vector((0, 0, -1))
    rotation = pythia_down.rotation_difference(to_target)

    tracks = []
    earth_hits = []

    with open(filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                s_local = Vector((float(row['x_start']) * SCALE,
                                  float(row['y_start']) * SCALE,
                                  float(row['z_start']) * SCALE))
                e_local = Vector((float(row['x_end']) * SCALE,
                                  float(row['y_end']) * SCALE,
                                  float(row['z_end']) * SCALE))
            except Exception:
                continue

            d_local = e_local - s_local
            if d_local.length <= 0.01 or d_local.z > 0:
                continue

            s_rot = s_local.copy(); s_rot.rotate(rotation)
            e_rot = e_local.copy(); e_rot.rotate(rotation)

            start = s_rot + ORIGIN_OFFSET
            end0  = e_rot + ORIGIN_OFFSET
            end   = start + (end0 - start) * TRACK_LENGTH_SCALE

            if earth_obj is not None:
                ray_dir = (end - start).normalized()
                ray_len = (end - start).length
                m_inv = earth_obj.matrix_world.inverted()
                o_local = m_inv @ start
                d_local_earth = (m_inv.to_3x3() @ ray_dir).normalized()
                hit, loc, _, _ = earth_obj.ray_cast(o_local, d_local_earth, distance=ray_len)
                if hit:
                    end = earth_obj.matrix_world @ loc
                    earth_hits.append(end)
                    tracks.append({'start': start, 'end': end})
            else:
                tracks.append({'start': start, 'end': end})

    filtered = []
    for t in tracks:
        if any((t['start'] - h).length < COLLISION_TOLERANCE for h in earth_hits):
            continue
        filtered.append(t)
    tracks = filtered

    # Beam object
    beam_curve = bpy.data.curves.new(f"IncomingRay_OverShoulder_{evt_idx}", 'CURVE')
    beam_curve.dimensions = '3D'
    beam_curve.bevel_depth = 0.002
    beam_curve.bevel_resolution = 4
    beam_curve.resolution_u = 12

    spline = beam_curve.splines.new('BEZIER')
    spline.bezier_points.add(1)
    for i in range(2):
        spline.bezier_points[i].handle_left_type  = 'VECTOR'
        spline.bezier_points[i].handle_right_type = 'VECTOR'

    beam = bpy.data.objects.new(f"IncomingRay_OverShoulder_{evt_idx}", beam_curve)
    evt_col.objects.link(beam)

    # Beam material
    beam_mat = bpy.data.materials.new(f"BeamMat_OverShoulder_{evt_idx}")
    beam_mat.use_nodes = True
    nodes = beam_mat.node_tree.nodes
    links = beam_mat.node_tree.links
    nodes.clear()
    beam_emission = nodes.new('ShaderNodeEmission')
    beam_output   = nodes.new('ShaderNodeOutputMaterial')
    beam_emission.inputs['Color'].default_value = (1.0, 0.3, 0.3, 1.0)
    beam_emission.inputs['Strength'].default_value = OVERSHOULDER_BEAM_BRIGHTNESS
    links.new(beam_emission.outputs[0], beam_output.inputs[0])
    beam.data.materials.append(beam_mat)

    # Beam timing
    t0 = float(start_frame)
    t_beam_end = t0 + BEAM_GROW_FRAMES
    t_beam_hold_end = t_beam_end + HOLD_FRAMES
    t_beam_fade_end = t_beam_hold_end + FADE_DURATION_FRAMES

    # Animate beam endpoints
    spline.bezier_points[0].co = far_pt
    spline.bezier_points[0].keyframe_insert(data_path="co", frame=t0)
    spline.bezier_points[0].co = far_pt
    spline.bezier_points[0].keyframe_insert(data_path="co", frame=t_beam_end)

    spline.bezier_points[1].co = far_pt
    spline.bezier_points[1].keyframe_insert(data_path="co", frame=t0)
    spline.bezier_points[1].co = ORIGIN_OFFSET
    spline.bezier_points[1].keyframe_insert(data_path="co", frame=t_beam_end)

    # Beam emission fade
    beam_emission.inputs['Strength'].default_value = OVERSHOULDER_BEAM_BRIGHTNESS
    beam_emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t0)
    beam_emission.inputs['Strength'].default_value = OVERSHOULDER_BEAM_BRIGHTNESS
    beam_emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_beam_hold_end)
    beam_emission.inputs['Strength'].default_value = 0.0
    beam_emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_beam_fade_end)

    # Hide after fade
    beam.hide_render = False;   beam.keyframe_insert(data_path="hide_render",   frame=t_beam_fade_end)
    beam.hide_render = True;    beam.keyframe_insert(data_path="hide_render",   frame=t_beam_fade_end + 1)
    beam.hide_viewport = False; beam.keyframe_insert(data_path="hide_viewport", frame=t_beam_fade_end)
    beam.hide_viewport = True;  beam.keyframe_insert(data_path="hide_viewport", frame=t_beam_fade_end + 1)

    # Set linear interpolation for geometry, but smooth for emission fade
    set_linear_fcurves(beam_curve)
    set_linear_fcurves(beam)
    set_smooth_emission_fade(beam_mat)

    # Tracks
    track_data = []
    for track in tracks:
        L = (track['end'] - track['start']).length
        if not math.isfinite(L) or L <= 0:
            continue
        frames = L / max(1e-9, RAY_TIP_SPEED)
        track_data.append({'track': track, 'duration': frames})

    per_track_end_frames = []
    for i, td in enumerate(track_data):
        tr = td['track']
        grow_frames = td['duration']

        curve = bpy.data.curves.new(f"Track_OverShoulder_{evt_idx}_{i}", 'CURVE')
        curve.dimensions = '3D'
        curve.bevel_depth = 0.0005
        curve.bevel_resolution = 2

        sp = curve.splines.new('POLY')
        sp.points.add(1)
        sp.points[0].co = (*tr['start'], 1.0)
        sp.points[1].co = (*tr['end'],   1.0)

        obj = bpy.data.objects.new(f"Track_OverShoulder_{evt_idx}_{i}", curve)
        evt_col.objects.link(obj)

        mat = bpy.data.materials.new(f"Mat_OverShoulder_{evt_idx}_{i}")
        mat.use_nodes = True
        n = mat.node_tree.nodes; l = mat.node_tree.links
        n.clear()
        emission = n.new('ShaderNodeEmission')
        out = n.new('ShaderNodeOutputMaterial')
        emission.inputs['Color'].default_value = (0.3, 0.6, 1.0, 1.0)
        emission.inputs['Strength'].default_value = TRACK_BRIGHTNESS
        l.new(emission.outputs[0], out.inputs[0])
        obj.data.materials.append(mat)

        t_track_start = t_beam_end
        t_track_end   = t_track_start + grow_frames
        t_hold_end    = t_track_end + HOLD_FRAMES
        t_fade_end    = t_hold_end + FADE_DURATION_FRAMES
        per_track_end_frames.append(t_fade_end)

        curve.bevel_factor_end = 0.0
        curve.keyframe_insert(data_path="bevel_factor_end", frame=t_track_start)
        curve.bevel_factor_end = 1.0
        curve.keyframe_insert(data_path="bevel_factor_end", frame=t_track_end)

        emission.inputs['Strength'].default_value = TRACK_BRIGHTNESS
        emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_track_start)
        emission.inputs['Strength'].default_value = TRACK_BRIGHTNESS
        emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_hold_end)
        emission.inputs['Strength'].default_value = 0.0
        emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_fade_end)

        obj.hide_render = False;   obj.keyframe_insert(data_path="hide_render",   frame=t_track_start)
        obj.hide_render = False;   obj.keyframe_insert(data_path="hide_render",   frame=t_fade_end)
        obj.hide_render = True;    obj.keyframe_insert(data_path="hide_render",   frame=t_fade_end + 1)
        obj.hide_viewport = False; obj.keyframe_insert(data_path="hide_viewport", frame=t_track_start)
        obj.hide_viewport = False; obj.keyframe_insert(data_path="hide_viewport", frame=t_fade_end)
        obj.hide_viewport = True;  obj.keyframe_insert(data_path="hide_viewport", frame=t_fade_end + 1)

        set_linear_fcurves(curve)
        set_linear_fcurves(obj)
        set_smooth_emission_fade(mat)

    event_end = t_beam_fade_end
    if per_track_end_frames:
        event_end = max(event_end, max(per_track_end_frames))
    return int(math.ceil(event_end))

# Event builder (normal events)
def build_one_event(start_frame: int, evt_idx: int) -> int:
    evt_col = bpy.data.collections.new(f"CosmicRay_Event_{evt_idx:02d}")
    master_col.children.link(evt_col)
    filepath = random.choice(csv_files)
    print(f"[Event {evt_idx}] CSV: {os.path.basename(filepath)}")

    ORIGIN_OFFSET = rand_origin(R_RANGE, THETA_RANGE, PHI_RANGE)
    
    target_point = random_point_in_sphere(TARGET_SPHERE_RADIUS)
    to_target = (target_point - ORIGIN_OFFSET).normalized()
    far_pt = ORIGIN_OFFSET - to_target * FAR_DISTANCE

    pythia_down = Vector((0, 0, -1))
    rotation = pythia_down.rotation_difference(to_target)

    tracks = []
    earth_hits = []

    with open(filepath, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                s_local = Vector((float(row['x_start']) * SCALE,
                                  float(row['y_start']) * SCALE,
                                  float(row['z_start']) * SCALE))
                e_local = Vector((float(row['x_end']) * SCALE,
                                  float(row['y_end']) * SCALE,
                                  float(row['z_end']) * SCALE))
            except Exception:
                continue

            d_local = e_local - s_local
            if d_local.length <= 0.01 or d_local.z > 0:
                continue

            s_rot = s_local.copy(); s_rot.rotate(rotation)
            e_rot = e_local.copy(); e_rot.rotate(rotation)

            start = s_rot + ORIGIN_OFFSET
            end0  = e_rot + ORIGIN_OFFSET
            end   = start + (end0 - start) * TRACK_LENGTH_SCALE

            if earth_obj is not None:
                ray_dir = (end - start).normalized()
                ray_len = (end - start).length
                m_inv = earth_obj.matrix_world.inverted()
                o_local = m_inv @ start
                d_local_earth = (m_inv.to_3x3() @ ray_dir).normalized()
                hit, loc, _, _ = earth_obj.ray_cast(o_local, d_local_earth, distance=ray_len)
                if hit:
                    end = earth_obj.matrix_world @ loc
                    earth_hits.append(end)
                    tracks.append({'start': start, 'end': end})
            else:
                tracks.append({'start': start, 'end': end})

    filtered = []
    for t in tracks:
        if any((t['start'] - h).length < COLLISION_TOLERANCE for h in earth_hits):
            continue
        filtered.append(t)
    tracks = filtered

    beam_curve = bpy.data.curves.new(f"IncomingRay_{evt_idx}", 'CURVE')
    beam_curve.dimensions = '3D'
    beam_curve.bevel_depth = 0.002
    beam_curve.bevel_resolution = 4
    beam_curve.resolution_u = 12

    spline = beam_curve.splines.new('BEZIER')
    spline.bezier_points.add(1)
    for i in range(2):
        spline.bezier_points[i].handle_left_type  = 'VECTOR'
        spline.bezier_points[i].handle_right_type = 'VECTOR'

    beam = bpy.data.objects.new(f"IncomingRay_{evt_idx}", beam_curve)
    evt_col.objects.link(beam)

    beam_mat = bpy.data.materials.new(f"BeamMat_{evt_idx}")
    beam_mat.use_nodes = True
    nodes = beam_mat.node_tree.nodes
    links = beam_mat.node_tree.links
    nodes.clear()
    beam_emission = nodes.new('ShaderNodeEmission')
    beam_output   = nodes.new('ShaderNodeOutputMaterial')
    beam_emission.inputs['Color'].default_value = (1.0, 0.2, 0.2, 1.0)
    beam_emission.inputs['Strength'].default_value = NORMAL_BEAM_BRIGHTNESS
    links.new(beam_emission.outputs[0], beam_output.inputs[0])
    beam.data.materials.append(beam_mat)

    t0 = float(start_frame)
    t_beam_end = t0 + BEAM_GROW_FRAMES
    t_beam_hold_end = t_beam_end + HOLD_FRAMES
    t_beam_fade_end = t_beam_hold_end + FADE_DURATION_FRAMES

    spline.bezier_points[0].co = far_pt
    spline.bezier_points[0].keyframe_insert(data_path="co", frame=t0)
    spline.bezier_points[0].co = far_pt
    spline.bezier_points[0].keyframe_insert(data_path="co", frame=t_beam_end)

    spline.bezier_points[1].co = far_pt
    spline.bezier_points[1].keyframe_insert(data_path="co", frame=t0)
    spline.bezier_points[1].co = ORIGIN_OFFSET
    spline.bezier_points[1].keyframe_insert(data_path="co", frame=t_beam_end)

    beam_emission.inputs['Strength'].default_value = NORMAL_BEAM_BRIGHTNESS
    beam_emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t0)
    beam_emission.inputs['Strength'].default_value = NORMAL_BEAM_BRIGHTNESS
    beam_emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_beam_hold_end)
    beam_emission.inputs['Strength'].default_value = 0.0
    beam_emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_beam_fade_end)

    beam.hide_render = False;   beam.keyframe_insert(data_path="hide_render",   frame=t_beam_fade_end)
    beam.hide_render = True;    beam.keyframe_insert(data_path="hide_render",   frame=t_beam_fade_end + 1)
    beam.hide_viewport = False; beam.keyframe_insert(data_path="hide_viewport", frame=t_beam_fade_end)
    beam.hide_viewport = True;  beam.keyframe_insert(data_path="hide_viewport", frame=t_beam_fade_end + 1)

    set_linear_fcurves(beam_curve)
    set_linear_fcurves(beam)
    set_smooth_emission_fade(beam_mat)

    track_data = []
    for track in tracks:
        L = (track['end'] - track['start']).length
        if not math.isfinite(L) or L <= 0:
            continue
        frames = L / max(1e-9, RAY_TIP_SPEED)
        track_data.append({'track': track, 'duration': frames})

    per_track_end_frames = []
    for i, td in enumerate(track_data):
        tr = td['track']
        grow_frames = td['duration']

        curve = bpy.data.curves.new(f"Track_{evt_idx}_{i}", 'CURVE')
        curve.dimensions = '3D'
        curve.bevel_depth = 0.0005
        curve.bevel_resolution = 2

        sp = curve.splines.new('POLY')
        sp.points.add(1)
        sp.points[0].co = (*tr['start'], 1.0)
        sp.points[1].co = (*tr['end'],   1.0)

        obj = bpy.data.objects.new(f"Track_{evt_idx}_{i}", curve)
        evt_col.objects.link(obj)

        mat = bpy.data.materials.new(f"Mat_{evt_idx}_{i}")
        mat.use_nodes = True
        n = mat.node_tree.nodes; l = mat.node_tree.links
        n.clear()
        emission = n.new('ShaderNodeEmission')
        out = n.new('ShaderNodeOutputMaterial')
        emission.inputs['Color'].default_value = (0.3, 0.6, 1.0, 1.0)
        emission.inputs['Strength'].default_value = TRACK_BRIGHTNESS
        l.new(emission.outputs[0], out.inputs[0])
        obj.data.materials.append(mat)

        t_track_start = t_beam_end
        t_track_end   = t_track_start + grow_frames
        t_hold_end    = t_track_end + HOLD_FRAMES
        t_fade_end    = t_hold_end + FADE_DURATION_FRAMES
        per_track_end_frames.append(t_fade_end)

        curve.bevel_factor_end = 0.0
        curve.keyframe_insert(data_path="bevel_factor_end", frame=t_track_start)
        curve.bevel_factor_end = 1.0
        curve.keyframe_insert(data_path="bevel_factor_end", frame=t_track_end)

        emission.inputs['Strength'].default_value = TRACK_BRIGHTNESS
        emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_track_start)
        emission.inputs['Strength'].default_value = TRACK_BRIGHTNESS
        emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_hold_end)
        emission.inputs['Strength'].default_value = 0.0
        emission.inputs['Strength'].keyframe_insert(data_path="default_value", frame=t_fade_end)

        obj.hide_render = False;   obj.keyframe_insert(data_path="hide_render",   frame=t_track_start)
        obj.hide_render = False;   obj.keyframe_insert(data_path="hide_render",   frame=t_fade_end)
        obj.hide_render = True;    obj.keyframe_insert(data_path="hide_render",   frame=t_fade_end + 1)
        obj.hide_viewport = False; obj.keyframe_insert(data_path="hide_viewport", frame=t_track_start)
        obj.hide_viewport = False; obj.keyframe_insert(data_path="hide_viewport", frame=t_fade_end)
        obj.hide_viewport = True;  obj.keyframe_insert(data_path="hide_viewport", frame=t_fade_end + 1)

        set_linear_fcurves(curve)
        set_linear_fcurves(obj)
        set_smooth_emission_fade(mat)

    event_end = t_beam_fade_end
    if per_track_end_frames:
        event_end = max(event_end, max(per_track_end_frames))

    return int(math.ceil(event_end))

# Build loop of events
current = scene.frame_start
evt_idx = 1

# Create ONE over-the-shoulder event at the beginning (frame 1)
event_end = build_over_shoulder_event(current, 0)
current = event_end + EVENT_GAP_FRAMES
evt_idx = 1

# Then continue with normal events
while current < SCENE_FRAMES - 10:
    event_end = build_one_event(current, evt_idx)
    next_start = event_end + EVENT_GAP_FRAMES
    if next_start <= current:
        next_start = current + int(BEAM_GROW_FRAMES + HOLD_FRAMES + FADE_DURATION_FRAMES + EVENT_GAP_FRAMES)
    if next_start >= SCENE_FRAMES:
        break
    current = next_start
    evt_idx += 1

scene.frame_end = SCENE_FRAMES
scene.frame_current = scene.frame_start

print("\n" + "="*60)
print(f"✓ Built 1 over-the-shoulder + {evt_idx-1} normal events")
print(f"  Emission brightness reduced for subtler effect:")
print(f"    Over-shoulder beam: {OVERSHOULDER_BEAM_BRIGHTNESS}")
print(f"    Normal beam: {NORMAL_BEAM_BRIGHTNESS}")
print(f"    Particle tracks: {TRACK_BRIGHTNESS}")
print(f"  Smooth bezier fade-out applied to all emissions")
print("="*60)