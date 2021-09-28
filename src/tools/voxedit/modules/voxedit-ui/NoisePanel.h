/**
 * @file
 */

#pragma once

namespace voxedit {

class NoisePanel {
private:
	struct NoiseData {
		int octaves = 4;
		float frequency = 0.01f;
		float lacunarity = 2.0f;
		float gain = 0.5f;
	};
	NoiseData _noiseData;

public:
	void update(const char *title);
};

}
