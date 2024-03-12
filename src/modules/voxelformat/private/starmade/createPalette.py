#!/usr/bin/env python3

# This palette is generated from the BlockConfig.xml file and the BlockTypes.properties file as well
# as the textures t00[0-2].png from starmadetools
#
# BlockConfig.xml
# BlockTypes.properties
# t002.png
# t001.png
# t000.png
#
# ./createPalette.py > SMPalette.h

import xml.etree.ElementTree as ET
from PIL import Image

def parse_properties_file(file_path):
    properties = {}
    with open(file_path, 'r') as file:
        for line in file:
            line = line.strip()
            if line == '' or line.startswith('#'):
                continue
            key, value = line.strip().split('=')
            properties[key.strip()] = value.strip()
    return properties

def texture_color_lookup(texture_id):
    texture_no = int(int(texture_id) / 256)
    texture_name = f"t{texture_no:03d}.png"

    try:
        # Load the PNG image
        image = Image.open(texture_name)

        # Get the size of the image
        width, height = image.size

        j = int(int(texture_id) % 256 % 16)
        k = int(int(texture_id) % 256 / 16)
        hScale = int(width / 16)
        vScale = int(height / 16)

        left = j*hScale
        top = k*vScale
        # right = left + hScale
        # bottom = top + vScale

        # Get the center pixel coordinates
        center_x = left + hScale // 2
        center_y = top + vScale // 2

        # Get the RGBA value of the center pixel
        center_rgba = image.getpixel((center_x, center_y))

        return center_rgba
    except FileNotFoundError:
        print(f"Error: Texture file '{texture_name}' not found.")
        return None

def rgba_out(rgba):
    return f"core::RGBA({rgba[0]}, {rgba[1]}, {rgba[2]}, {rgba[3]})"

def float_color_out(color):
    color = color.split(",")
    return f"core::RGBA({int(float(color[0])*255.0)}, {int(float(color[1])*255.0)}, {int(float(color[2])*255.0)}, {int(float(color[3])*255.0)})"

def print_colors(root, block_properties, emit):
    if emit:
        print("static const BlockColor BLOCKEMITCOLOR[]{")
    else:
        print("static const BlockColor BLOCKCOLOR[]{")

    for block_type, block_id in block_properties.items():
        for block_node in root.iter('Block'):
            if block_node.attrib.get('type') == block_type:
                texture_id = block_node.attrib.get('textureId')
                light_source = block_node.find('LightSource')
                if emit:
                    if light_source.text == "false":
                        continue
                    light_source_color = block_node.find('LightSourceColor')
                    print(f"	{{ {block_id}, {float_color_out(light_source_color.text)} }}, // emit for {block_type}")
                else:
                    rgba = texture_color_lookup(texture_id)
                    if rgba is None:
                        raise SystemError(f"Error: Texture file for block type '{block_type}' not found.")
                    print(f"	{{ {block_id}, {rgba_out(rgba)} }}, // {block_type}")
    print("};")

if __name__ == "__main__":
    block_properties = parse_properties_file("BlockTypes.properties")
    tree = ET.parse("BlockConfig.xml")
    root = tree.getroot()

    print("/**")
    print(" * @file")
    print(" */")
    print("")
    print("#pragma once")
    print("")
    print("#include \"core/RGBA.h\"")
    print("")
    print("namespace voxelformat {")
    print("")
    print("struct BlockColor {")
    print("	int blockId;")
    print("	core::RGBA color;")
    print("};")
    print_colors(root, block_properties, False)
    print_colors(root, block_properties, True)
    print("}; // namespace voxelformat")
