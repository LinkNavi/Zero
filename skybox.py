# exr_to_cubemap.py
import numpy as np
import OpenEXR
import Imath
import cv2
from PIL import Image

def exr_to_cubemap(exr_path, output_dir, size=None):
    # Read EXR
    exr = OpenEXR.InputFile(exr_path)
    header = exr.header()
    dw = header['dataWindow']
    width = dw.max.x - dw.min.x + 1
    height = dw.max.y - dw.min.y + 1
    
    # Auto-detect face size from equirectangular dimensions
    # For equirect: width = 2 * height, face_size = height / 2
    if size is None:
        size = height // 2  # Use half of height as face size
        print(f"Auto-detected face size: {size}x{size} (from {width}x{height} equirect)")
    
    # Read RGB channels
    FLOAT = Imath.PixelType(Imath.PixelType.FLOAT)
    r = np.frombuffer(exr.channel('R', FLOAT), dtype=np.float32).reshape(height, width)
    g = np.frombuffer(exr.channel('G', FLOAT), dtype=np.float32).reshape(height, width)
    b = np.frombuffer(exr.channel('B', FLOAT), dtype=np.float32).reshape(height, width)
    
    # Stack to RGB
    hdr = np.stack([r, g, b], axis=-1)
    
    # Tonemap HDR -> LDR
    hdr = np.clip(hdr * 1.5, 0, 1)  # Simple exposure
    hdr = np.power(hdr, 1/2.2)  # Gamma correction
    ldr = (hdr * 255).astype(np.uint8)
    
    # Convert equirectangular to cubemap faces
    h, w = ldr.shape[:2]
    face_size = size
    
    def sample_equirect(x, y, z):
        vec = np.array([x, y, z])
        vec = vec / np.linalg.norm(vec)
        
        u = 0.5 + np.arctan2(vec[0], vec[2]) / (2 * np.pi)
        v = 0.5 - np.arcsin(vec[1]) / np.pi
        
        px = int(u * (w - 1))
        py = int(v * (h - 1))
        return ldr[py, px]
    
    faces = {}
    
    print(f"Generating {face_size}x{face_size} cubemap faces...")
    
    # +X (right)
    face = np.zeros((face_size, face_size, 3), dtype=np.uint8)
    for i in range(face_size):
        for j in range(face_size):
            u = (j + 0.5) / face_size * 2 - 1
            v = (i + 0.5) / face_size * 2 - 1
            face[i, j] = sample_equirect(1, -v, -u)
    faces['right'] = face
    
    # -X (left)
    face = np.zeros((face_size, face_size, 3), dtype=np.uint8)
    for i in range(face_size):
        for j in range(face_size):
            u = (j + 0.5) / face_size * 2 - 1
            v = (i + 0.5) / face_size * 2 - 1
            face[i, j] = sample_equirect(-1, -v, u)
    faces['left'] = face
    
    # +Y (top)
    face = np.zeros((face_size, face_size, 3), dtype=np.uint8)
    for i in range(face_size):
        for j in range(face_size):
            u = (j + 0.5) / face_size * 2 - 1
            v = (i + 0.5) / face_size * 2 - 1
            face[i, j] = sample_equirect(u, 1, v)
    faces['top'] = face
    
    # -Y (bottom)
    face = np.zeros((face_size, face_size, 3), dtype=np.uint8)
    for i in range(face_size):
        for j in range(face_size):
            u = (j + 0.5) / face_size * 2 - 1
            v = (i + 0.5) / face_size * 2 - 1
            face[i, j] = sample_equirect(u, -1, -v)
    faces['bottom'] = face
    
    # +Z (front)
    face = np.zeros((face_size, face_size, 3), dtype=np.uint8)
    for i in range(face_size):
        for j in range(face_size):
            u = (j + 0.5) / face_size * 2 - 1
            v = (i + 0.5) / face_size * 2 - 1
            face[i, j] = sample_equirect(u, -v, 1)
    faces['front'] = face
    
    # -Z (back)
    face = np.zeros((face_size, face_size, 3), dtype=np.uint8)
    for i in range(face_size):
        for j in range(face_size):
            u = (j + 0.5) / face_size * 2 - 1
            v = (i + 0.5) / face_size * 2 - 1
            face[i, j] = sample_equirect(-u, -v, -1)
    faces['back'] = face
    
    # Save
    import os
    os.makedirs(output_dir, exist_ok=True)
    for name, img in faces.items():
        img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        Image.fromarray(img_rgb).save(f"{output_dir}/{name}.jpg", quality=95)
        print(f"  Saved {name}.jpg")
    
    print(f"✓ Saved 6 cubemap faces to {output_dir}")

# Usage - size parameter is now optional (auto-detects from EXR)
exr_to_cubemap("skybox.exr", "textures/skybox")

# Or override with specific size:
# exr_to_cubemap("skybox.exr", "textures/skybox", size=2048)
