// Shared C++/GLSL scene records; define VENGICPP when including from C++.
// Float fields map directly to RGBA32F texels. Layout assertions are in GpuSceneData.h.

#ifdef VENGICPP
namespace voxelpathtracer {
#endif

// One dense material. Uploaded as MaterialRows RGBA32F texels, width = MaterialRows,
// height = materialCount, y = materialId - 1. Albedo is a separate flat RGBA8 row.
struct GpuMaterial {
	float type;
	float roughness;
	float metallic;
	float emission;
	float ior;
	float transparency;
	float specular;
	float attenuation;
	float flux;
	float density;
	float phase;
	float media;
	float ldr;
	float sp;
	float mediatype;
	float transmits;
};

// One emit voxel. Two RGBA32F texels (width = 2, height = count).
struct GpuEmitLight {
	float cellx;
	float celly;
	float cellz;
	float aliasThreshold;
	float radianceR;
	float radianceG;
	float radianceB;
	float aliasIndex;
};

#ifdef VENGICPP
}
#endif
