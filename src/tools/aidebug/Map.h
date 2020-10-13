/**
 * @file
 */

#include <glm/vec2.hpp>

class Map {
private:
	float _zoom = 1.0f;
	glm::vec2 _dbgMapOffset { 0.0f, 0.0f };
	glm::ivec2 _mins {0};
	glm::ivec2 _maxs {0};
	glm::ivec2 calculateOffsetPos(float x, float y, const glm::ivec2& center) const;

public:
	void scroll(const glm::ivec2& amount);
	void setMinsMaxs(const glm::ivec2& mins, const glm::ivec2& maxs);
	void reset();
	void centerAtEntPos(float x, float y);
	void zoomAtMapPos(float x, float y, float deltaZoom);
	float zoom() const;
	glm::ivec2 entPosToMap(float x, float y) const;
	glm::ivec2 mapToEntPos(float x, float y) const;
	bool isVisible(const glm::ivec2& pos, const glm::ivec2& mapMins, const glm::ivec2& mapMaxs) const;
};
