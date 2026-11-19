# Material

Each color entry in the [palette](Palette.md) can have several material properties. The GPU pathtracer shades the material types and properties. The forward renderer uses `VoxelType` and palette alpha for transparency; other properties are kept for import/export.

## Materials

> The material support in vengi is modelled after magicavoxel.

The following material names are used by vengi and a few of them are exported to the GLTF-[format](Formats.md).

| Material name         | Pathtracer                                      | GLTF mapping                                                 |
| --------------------- | ----------------------------------------------- | ------------------------------------------------------------ |
| `metal`               | metal / blend (also tints surface BRDF)         | pbrMetallicRoughness.metallicFactor                          |
| `roughness`           | all surface types; combined with `sp` via max   | pbrMetallicRoughness.roughnessFactor                         |
| `specular`            | diffuse / blend (metal always uses full specular) | KHR_materials_specular (optional pbrSpecularGlossiness fallback) |
| `indexOfRefraction`   | glass / blend / media                           | KHR_materials_ior                                            |
| `attenuation`         | glass / media beer-lambert                      | KHR_materials_volume.attenuationDistance (= 1 / attenuation) |
| `flux`                | emit energy `color * emit * 2^flux`             | VENGI_materials.flux                                         |
| `emit`                | emit / media-emit                               | emissiveFactor                                               |
| `lowDynamicRange`     | mix emit toward non-HDR (`color * emit`)        | VENGI_materials.lowDynamicRange                              |
| `density`             | volume scatter inside glass / media             | VENGI_materials.density                                      |
| `sp`                  | frost: raises effective roughness               | VENGI_materials.sp                                           |
| `phase`               | HG asymmetry in `[0,1]` (`0.5` = isotropic)     | VENGI_materials.phase                                        |
| `media`               | 0 absorb, 1 scatter, 2 emit, 3 sss              | VENGI_materials.media                                        |

MaterialTypes:

| Type    | Pathtracer                                                           |
| ------- | -------------------------------------------------------------------- |
| Diffuse | Lambert, optional specular                                           |
| Metal   | GGX-like reflection from `metal` / `roughness` (full specular lobe)  |
| Glass   | Dielectric refraction, IOR, palette alpha, attenuation               |
| Emit    | `color * emit * 2^flux`, `ldr` mix                                   |
| Blend   | Stochastic mix of glass vs metal vs diffuse from alpha and `metal`   |
| Media   | Treated as glass entry; volume absorb / scatter / emit / sss from `media`, `density`, `phase` |

MagicaVoxel `_ior` is stored as `n = 1 + _ior` when `_ior < 1`. `_ri` overwrites that with the real refractive index.

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
