// Texture accessors for scene records uploaded by PathTracer::createScene().
// Requires _pathtracershared.glsl and the shared structs in _gpuscene.inc.

uint materialId(uint payload) {
	return payload & uint(VoxelMaterialIdMask);
}

bool payloadTransmits(uint payload) {
	return (payload & uint(VoxelFlagTransmits)) != 0u;
}

bool payloadEmits(uint payload) {
	return (payload & uint(VoxelFlagEmit)) != 0u;
}

// u_palette: RGBA8 albedo, width=materialCount, height=1. x = materialId - 1.
vec4 fetchPalette(uint payload) {
	return texelFetch(u_palette, ivec2(int(materialId(payload) - 1u), 0), 0);
}

int gpuMaterialIndex(uint payload) {
	return int(materialId(payload) - 1u);
}

// u_materials: RGBA32F, width=MaterialRows, height=materialCount.
// y = materialId - 1. One GpuMaterial is MaterialRows consecutive texels (AOS).
vec4 fetchMaterialRow(uint payload, int row) {
	return texelFetch(u_materials, ivec2(row, gpuMaterialIndex(payload)), 0);
}

GpuMaterial fetchMaterial(uint payload) {
	vec4 r0 = fetchMaterialRow(payload, 0);
	vec4 r1 = fetchMaterialRow(payload, 1);
	vec4 r2 = fetchMaterialRow(payload, 2);
	vec4 r3 = fetchMaterialRow(payload, 3);
	GpuMaterial m;
	m.type = r0.x;
	m.roughness = r0.y;
	m.metallic = r0.z;
	m.emission = r0.w;
	m.ior = r1.x;
	m.transparency = r1.y;
	m.specular = r1.z;
	m.attenuation = r1.w;
	m.flux = r2.x;
	m.density = r2.y;
	m.phase = r2.z;
	m.media = r2.w;
	m.ldr = r3.x;
	m.sp = r3.y;
	m.mediatype = r3.z;
	m.transmits = r3.w;
	return m;
}

GpuMaterial gpuMaterialFloor() {
	GpuMaterial m;
	m.type = float(MaterialDiffuse);
	m.roughness = 0.9;
	m.metallic = 0.0;
	m.emission = 0.0;
	m.ior = 1.5;
	m.transparency = 0.0;
	m.specular = 0.0;
	m.attenuation = 0.0;
	m.flux = 0.0;
	m.density = 0.0;
	m.phase = 0.0;
	m.media = 0.0;
	m.ldr = 0.0;
	m.sp = 0.0;
	m.mediatype = 0.0;
	m.transmits = 0.0;
	return m;
}

// u_lights: RGBA32F, width=2, height=count. Position and linear radiance.
GpuEmitLight fetchEmitLight(int lightIndex) {
	vec4 e = texelFetch(u_lights, ivec2(0, lightIndex), 0);
	vec4 radiance = texelFetch(u_lights, ivec2(1, lightIndex), 0);
	GpuEmitLight l;
	l.cellx = e.x;
	l.celly = e.y;
	l.cellz = e.z;
	l.aliasThreshold = e.w;
	l.radianceR = radiance.r;
	l.radianceG = radiance.g;
	l.radianceB = radiance.b;
	l.aliasIndex = radiance.w;
	return l;
}

ivec3 gpuEmitLightCell(GpuEmitLight l) {
	return ivec3(vec3(l.cellx, l.celly, l.cellz) + 0.5);
}

int gpuMaterialType(GpuMaterial m) {
	return int(m.type + 0.5);
}

int gpuMaterialMediaType(GpuMaterial m) {
	return int(m.mediatype + 0.5);
}
