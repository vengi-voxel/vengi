#include "NoiseNode.h"
#include "noise/Noise.h"
#include "core/App.h"

static const char* NoiseTypeStr[] = {
	"double noise",
	"simplex noise",
	"ridged noise",
	"flow noise (rot. gradients)",
	"fbm",
	"fbm cascade",
	"fbm analytical derivatives",
	"flow noise fbm (time)",
	"ridged multi fractal (time)",
	"ridged multi fractal",
	"ridged multi fractal cascade",
	"iq noise",
	"analytical derivatives",
	"noise curl noise (time)",
	"worley noise",
	"worley noise fbm",
	"voronoi",
	"swissTurbulence",
	"jordanTurbulence"
};
static constexpr int numValues = (int)SDL_arraysize(NoiseTypeStr);
static_assert(int(NoiseType::Max) == numValues, "Array size doesn't match NoiseType::Max value");

static inline unsigned long millis() {
	return core::App::getInstance()->timeProvider()->currentTime();
}

bool NoiseNode::GetNoiseTypeFromEnumIndex(void*, int value, const char** pTxt) {
	if (!pTxt) {
		return false;
	}
	if (value >= 0 && value < numValues) {
		*pTxt = NoiseTypeStr[value];
	} else {
		*pTxt = "UNKNOWN";
	}
	return true;
}

float NoiseNode::getNoise(int x, int y) {
	float ridgedOffset = 1.0f;
	const NoiseType noiseType = NoiseType(noiseTypeIndex);
	const glm::vec2 position(offset + x * frequency, offset + y * frequency);
	switch (noiseType) {
	case NoiseType::doubleNoise: {
		const glm::ivec3 p3(position.x, position.y, 0);
		return noise::doubleValueNoise(p3, 0);
	}
	case NoiseType::simplexNoise:
		return noise::noise(position);
	case NoiseType::ridgedNoise:
		return noise::ridgedNoise(position);
	case NoiseType::flowNoise:
		return noise::flowNoise(position, millis());
	case NoiseType::fbm:
		return noise::fBm(position, octaves, lacunarity, gain);
	case NoiseType::fbmCascade:
		return noise::fBm(noise::fBm(position));
	case NoiseType::fbmAnalyticalDerivatives:
		return noise::fBm(noise::dfBm(position));
	case NoiseType::flowNoiseFbm: {
		const glm::vec3 p3(position, millis() * 0.1f);
		const float fbm = noise::fBm(p3, octaves, lacunarity, gain);
		return noise::flowNoise(position + fbm, millis());
	}
	case NoiseType::ridgedMFTime: {
		const glm::vec3 p3(position, millis() * 0.1f);
		return noise::ridgedMF(p3, ridgedOffset, octaves, lacunarity, gain);
	}
	case NoiseType::ridgedMF:
		return noise::ridgedMF(position, ridgedOffset, octaves, lacunarity, gain);
	case NoiseType::ridgedMFCascade: {
		const float n = noise::ridgedMF(position, ridgedOffset, octaves, lacunarity, gain);
		return noise::ridgedMF(n, ridgedOffset, octaves, lacunarity, gain);
	}
	case NoiseType::iqNoise:
		return noise::iqMatfBm(position, octaves, glm::mat2(2.3f, -1.5f, 1.5f, 2.3f), gain);
	case NoiseType::analyticalDerivatives: {
		const glm::vec3& n = noise::dnoise(position);
		return (n.y + n.z) * 0.5f;
	}
	case NoiseType::noiseCurlNoise: {
		const glm::vec2& n = noise::curlNoise(position, millis());
		return noise::noise(glm::vec2(position.x + n.x, position.y + n.x));
	}
	case NoiseType::voronoi: {
		const bool enableDistance = false;
		const int seed = 0;
		const glm::dvec3 p3(position.x, position.y, 0.0);
		return noise::voronoi(p3, enableDistance, 1.0, seed);
	}
	case NoiseType::worleyNoise:
		return noise::worleyNoise(position);
	case NoiseType::worleyNoiseFbm:
		return noise::worleyfBm(position, octaves, lacunarity, gain);
	case NoiseType::swissTurbulence:
		return noise::swissTurbulence(position, 0.0f, octaves, lacunarity, gain);
	case NoiseType::jordanTurbulence:
		// float gain0, float gain, float warp0, float warp, float damp0, float damp, float damp_scale;
		return noise::jordanTurbulence(position, 0.0f, octaves, lacunarity, gain);
	case NoiseType::Max:
		break;
	}
	return 0.0f;
}

void NoiseNode::getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut, ImU32& defaultTitleBgColorOut, float& defaultTitleBgColorGradientOut) const {
	defaultTitleTextColorOut = IM_COL32(220, 220, 220, 255);
	defaultTitleBgColorOut = IM_COL32(125, 35, 0, 255);
	defaultTitleBgColorGradientOut = -1.f;
}

NoiseNode* NoiseNode::Create(const ImVec2& pos, ImGui::NodeGraphEditor& nge) {
	NoiseNode* node = imguiAlloc<NoiseNode>();
	node->setup(nge, pos, nullptr, "noise", NodeType::Noise);
	node->fields.addField(&node->frequency, 1, "Frequency", "Noise frequency", 8, 0, 1);
	node->fields.addField(&node->offset, 1, "Offset", "Noise offset", 8, 0, 1000);
	node->fields.addField(&node->lacunarity, 1, "Lacunarity", "Noise lacunarity", 8, 0, 10);
	node->fields.addField(&node->octaves, 1, "Octaves", "Noise octaves", 0, 1, 8);
	node->fields.addField(&node->gain, 1, "Gain", "Noise gain", 8, 0, 20);
	node->fields.addFieldEnum(&node->noiseTypeIndex, numValues, &GetNoiseTypeFromEnumIndex, "Type", "Choose noise type");
	node->noiseTypeIndex = 1;
	return node;
}
