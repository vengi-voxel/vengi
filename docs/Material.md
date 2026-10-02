# Material

Each color entry in the [palette](Palette.md) can have several material properties. Most of them are not handled in the vengi renderers, but can be useful when exporting the voxels to other [formats](Formats.md).

## Materials

> The material support in vengi is modelled after magicavoxel.

The following material names are used by vengi and a few of them are exported to the GLTF-[format](Formats.md).

| Material name         | GLTF mapping                                               |
| --------------------- | ---------------------------------------------------------- |
| `metal`               | pbrMetallicRoughness.metallicFactor                        |
| `roughness`           | pbrMetallicRoughness.roughnessFactor                       |
| `specular`            | KHR_materials_specular (fallback: KHR_materials_pbrSpecularGlossiness) |
| `indexOfRefraction`   | KHR_materials_ior                                          |
| `attenuation`         | KHR_materials_volume.attenuationDistance (= 1 / attenuation) |
| `flux`                | VENGI_materials.flux                                       |
| `emit`                | emissiveFactor                                             |
| `lowDynamicRange`     | VENGI_materials.lowDynamicRange                            |
| `density`             | VENGI_materials.density                                    |
| `sp`                  | VENGI_materials.sp                                         |
| `phase`               | VENGI_materials.phase                                      |
| `media`               | VENGI_materials.media                                      |

`MaterialType` (Diffuse / Metal / Glass / Emit / Blend / Media) has no stock glTF equivalent. On export it is stored in the `VENGI_materials` extension (`type`) together with the properties that have no stock glTF mapping, and restored on import when that extension is present. Older files that used `extras.vengi` are still read.

Scene graph node properties (Author, Title, and other key/value pairs) are stored on nodes and the scene in the `VENGI_properties` extension.

You can also modify these values via [scripting](LUAScript.md).

## GLTF extensions

Some of the material properties are exported to GLTF 2.0 or some of the extensions:

* [KHR_materials_ior](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_ior)
* [KHR_materials_volume](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_volume)
* [KHR_materials_specular](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_specular)
* [KHR_materials_pbrSpecularGlossiness](https://kcoley.github.io/glTF/extensions/2.0/Khronos/KHR_materials_pbrSpecularGlossiness) (optional fallback for specular; off by default)
* [KHR_materials_emissive_strength](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_materials_emissive_strength) (read on import for HDR scale; emit 0..1 uses core emissiveFactor only)
* `VENGI_materials` (material type and properties without a stock glTF mapping)
* `VENGI_properties` (scene graph node properties)
