/**
 * @file
 */

#include "Map.h"
#include "core/Common.h"

void Map::setMinsMaxs(const glm::ivec2& mins, const glm::ivec2& maxs) {
	_mins = mins;
	_maxs = maxs;
}

void Map::reset() {
	_dbgMapOffset = { 0.0f, 0.0f };
	_zoom = 1.0f;
}

void Map::centerAtEntPos(float x, float y) {
	_dbgMapOffset = calculateOffsetPos(x, y, (_maxs - _mins) / 2);
}

void Map::zoomAtMapPos(float x, float y, float deltaZoom) {
	const glm::ivec2 entPos = mapToEntPos(x, y);
	_zoom = core_max(0.01f, zoom() + deltaZoom);
	_dbgMapOffset = calculateOffsetPos(entPos.x, entPos.y, glm::ivec2(x, y));
}

void Map::scroll(const glm::ivec2& amount) {
	_dbgMapOffset += amount;
}

float Map::zoom() const {
	return _zoom;
}

glm::ivec2 Map::calculateOffsetPos(float x, float y, const glm::ivec2& center) const {
	return glm::ivec2(-x * zoom() + (float)center.x, -y * zoom() + (float)center.y);
}

glm::ivec2 Map::entPosToMap(float x, float y) const {
	return glm::ivec2(zoom() * (_dbgMapOffset.x + x), zoom() * (_dbgMapOffset.y + y));
}

glm::ivec2 Map::mapToEntPos(float x, float y) const {
	return glm::ivec2((x - _dbgMapOffset.x) / zoom(), (y - _dbgMapOffset.y) / zoom());
}

bool Map::isVisible(const glm::ivec2& pos, const glm::ivec2& mapMins, const glm::ivec2& mapMaxs) const {
	return pos.x > mapMins.x && pos.y > mapMins.y && pos.x < mapMaxs.x && pos.y < mapMaxs.y;
}
