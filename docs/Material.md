# Material

Each color entry in the [palette](Palette.md) can have several material properties. Most of them are not handled in the vengi renderers, but can be useful when exporting the voxels to other [formats](Formats.md).

## Materials

> The material support in vengi is modelled after magicavoxel.

The following material names are imported from magicavoxel and a few of them are exported to the GLTF-[format](Formats.md).

| Material name         | GLTF mapping                                               |
| --------------------- | ---------------------------------------------------------- |
| `metal`               | pbrMetallicRoughness.metallicFactor                        |
| `roughness`           | pbrMetallicRoughness.roughnessFactor                       |
| `specular`            | KHR_materials_specular (fallback: KHR_materials_pbrSpecularGlossiness) |
| `indexOfRefraction`   | KHR_materials_ior                                          |
| `attenuation`         | KHR_materials_volume.attenuationDistance (= 1 / attenuation) |
| `flux`                |                                                            |
| `emit`                | emissiveFactor                                             |
| `lowDynamicRange`     |                                                            |
| `density`             |                                                            |
| `sp`                  |                                                            |
| `phase`               |                                                            |
| `media`               |                                                            |

MagicaVoxel `MaterialType` (Diffuse / Metal / Glass / Emit / Blend / Media) has no stock glTF equivalent and is not reconstructed on import.

You can also modify these values via [scripting](LUAScript.md).

## GLTF extensions

Some of the material properties are exported to GLTF 2.0 or some of the extensions:

* [KHR_materials_ior](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_ior)
* [KHR_materials_volume](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_volume)
* [KHR_materials_specular](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular)
* [KHR_materials_pbrSpecularGlossiness](https://kcoley.github.io/glTF/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness) (optional fallback for specular; off by default)
* [KHR_materials_emissive_strength](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_emissive_strength) (read on import for HDR scale; MagicaVoxel emit 0..1 uses core emissiveFactor only)
