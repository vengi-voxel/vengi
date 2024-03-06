# Material

Each color entry in the [palette](Palette.md) can have several material properties. Most of them are not handled in the vengi renderers, but can be useful when exporting the voxels to other [formats](Formats.md).

## Materials

> The material support in vengi is modelled after magicavoxel.

The following material names are imported from magicavoxel and a few of them are exported to the GLTF-[format](Formats.md).

| Material name         | GLTF mapping                                               |
| --------------------- | ---------------------------------------------------------- |
| `metal`               | pbrMetallicRoughness                                       |
| `roughness`           | KHR_materials_pbrSpecularGlossiness, pbrMetallicRoughness  |
| `specular`            | KHR_materials_specular                                     |
| `indexOfRefraction`   | KHR_materials_ior                                          |
| `attenuation`         | KHR_materials_volume                                       |
| `flux`                |                                                            |
| `emit`                | Emission texture                                           |
| `lowDynamicRange`     |                                                            |
| `density`             | KHR_materials_pbrSpecularGlossiness                        |
| `sp`                  |                                                            |
| `glossiness`          | KHR_materials_pbrSpecularGlossiness                        |
| `media`               |                                                            |

You can also modify these values via [scripting](LUAScript.md).

## GLTF extensions

Some of the material properties are exported to GLTF 2.0 or some of the extensions:

* [KHR_materials_emissive_strength](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_emissive_strength)
* [KHR_materials_ior](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_ior)
* [KHR_materials_volume](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_volume)
* [KHR_materials_pbrSpecularGlossiness](https://kcoley.github.io/glTF/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness)
* [KHR_materials_specular](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular)
