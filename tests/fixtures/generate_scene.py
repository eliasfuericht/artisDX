"""Generate the small, self-contained GLB used by the GPU smoke test (stdlib only)."""
import json
from pathlib import Path
import struct
import zlib


def png(color):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    rows = b"".join(b"\0" + bytes(color) * 8 for _ in range(8))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 8, 8, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def generate():
    binary = bytearray()
    views = []

    def add(data):
        binary.extend(b"\0" * (-len(binary) % 4))
        views.append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(data)})
        binary.extend(data)
        return len(views) - 1

    positions = add(struct.pack("<12f", -0.8, -0.8, 0, 0.8, -0.8, 0, 0.8, 0.8, 0, -0.8, 0.8, 0))
    normals = add(struct.pack("<12f", *(0, 0, 1) * 4))
    uvs = add(struct.pack("<8f", 0, 0, 1, 0, 1, 1, 0, 1))
    indices = add(struct.pack("<6H", 0, 1, 2, 0, 2, 3))
    images = [add(png(color)) for color in ((200, 60, 30, 255), (0, 200, 0, 255), (128, 128, 255, 255))]
    binary.extend(b"\0" * (-len(binary) % 4))
    accessors = [
        {"bufferView": positions, "componentType": 5126, "count": 4, "type": "VEC3",
         "min": [-0.8, -0.8, 0], "max": [0.8, 0.8, 0]},
        {"bufferView": normals, "componentType": 5126, "count": 4, "type": "VEC3"},
        {"bufferView": uvs, "componentType": 5126, "count": 4, "type": "VEC2"},
        {"bufferView": indices, "componentType": 5123, "count": 6, "type": "SCALAR"},
    ]
    asset = {
        "asset": {"version": "2.0", "generator": "artisDX regression fixture"},
        "scene": 0, "scenes": [{"nodes": [0]}],
        "nodes": [{"name": "Parent", "children": [1, 2]},
                  {"name": "Textured", "mesh": 0, "translation": [-0.9, 0, 0]},
                  {"name": "FallbackBlend", "mesh": 1, "translation": [0.9, 0, 0], "scale": [0.8, 0.8, 1]}],
        "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
                                      "indices": 3, "material": material}]} for material in range(2)],
        "materials": [
            {"name": "TexturedPBR", "pbrMetallicRoughness": {"baseColorTexture": {"index": 0},
                "metallicRoughnessTexture": {"index": 1}, "metallicFactor": 0, "roughnessFactor": 0.8},
             "normalTexture": {"index": 2}},
            {"name": "FallbackBlend", "alphaMode": "BLEND", "pbrMetallicRoughness": {
                "baseColorFactor": [0.3, 0.8, 0.4, 0.6], "metallicFactor": 0, "roughnessFactor": 1}},
        ],
        "textures": [{"source": index} for index in range(3)],
        "images": [{"bufferView": view, "mimeType": "image/png"} for view in images],
        "accessors": accessors, "bufferViews": views, "buffers": [{"byteLength": len(binary)}],
    }
    description = json.dumps(asset, separators=(",", ":")).encode()
    description += b" " * (-len(description) % 4)
    glb = (struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(description) + 8 + len(binary))
           + struct.pack("<II", len(description), 0x4E4F534A) + description
           + struct.pack("<II", len(binary), 0x004E4942) + binary)
    Path(__file__).with_name("regression.glb").write_bytes(glb)


if __name__ == "__main__":
    generate()
