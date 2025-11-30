#!/usr/bin/env python3

# This palette is generated from the BlockConfig.xml file and the BlockTypes.properties file as well
# as the textures t00[0-2].png from starmade
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
import sys

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
        right = left + hScale
        bottom = top + vScale

        sub_texture = image.crop((left, top, right, bottom))
        sub_texture_colors = sub_texture.getdata()
        alpha_total_r = alpha_total_g = alpha_total_b = alpha_total_a = 0
        opaque_total_r = opaque_total_g = opaque_total_b = opaque_total_a = 0
        total_alpha = 0
        total_opaque = 0
        for rgba in sub_texture_colors:
            r, g, b, a = rgba
            if a == 0:
                continue
            if a < 255:
                alpha_total_r += r
                alpha_total_g += g
                alpha_total_b += b
                alpha_total_a += a
                total_alpha += 1
            else:
                opaque_total_r += r
                opaque_total_g += g
                opaque_total_b += b
                opaque_total_a += a
                total_opaque += 1

        if total_alpha / 10 > total_opaque:
            total_r = alpha_total_r
            total_g = alpha_total_g
            total_b = alpha_total_b
            min_alpha = 50
            total_a = min_alpha if alpha_total_a // total_alpha < min_alpha else alpha_total_a // total_alpha
            total_colors = total_alpha
        else:
            total_r = opaque_total_r
            total_g = opaque_total_g
            total_b = opaque_total_b
            total_a = 255
            total_colors = total_opaque

        if total_colors == 0:
            return (0, 0, 0, 255)

        # TODO: don't use the average color but the most significant color - if there are alpha values included and fully opaque,
        # the alpha values should be ignored
        avg_r = total_r // total_colors
        avg_g = total_g // total_colors
        avg_b = total_b // total_colors
        avg_a = total_a

        # Return the average color as RGBA tuple
        return (avg_r, avg_g, avg_b, avg_a)
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
            if block_node.attrib.get('type') != block_type:
                continue
            print(f"block_type {block_type}", file=sys.stderr)
            texture_ids = block_node.attrib.get('textureId')
            light_source = block_node.find('LightSource')
            if emit:
                if light_source.text == "false":
                    continue
                light_source_color = block_node.find('LightSourceColor')
                print(f"	{{ {block_id}, {float_color_out(light_source_color.text)} }}, // emit for {block_type}")
            else:
                # if texture_ids is an array, pick the first one
                if ',' in texture_ids:
                    texture_ids = texture_ids.split(',')[0]
                rgba = texture_color_lookup(texture_ids)
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
    print("#include \"color/RGBA.h\"")
    print("#include \"palette/Palette.h\"")
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
