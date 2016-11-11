#include "SelectionHandler.h"
#include "select/Edge.h"
#include "select/LineHorizontal.h"
#include "select/LineVertical.h"
#include "select/Same.h"
#include "select/Single.h"
#include <SDL.h>

namespace voxedit {

static const struct Selection {
	SelectType type;
	selections::Select& select;
} selectionsArray[] = {
	{SelectType::Single,			selections::Single::get()},
	{SelectType::Same,				selections::Same::get()},
	{SelectType::LineVertical,		selections::LineVertical::get()},
	{SelectType::LineHorizontal,	selections::LineHorizontal::get()},
	{SelectType::Edge,				selections::Edge::get()}
};
static_assert(SDL_arraysize(selectionsArray) == std::enum_value(SelectType::Max), "Array size doesn't match selection modes");

bool SelectionHandler::select(const voxel::RawVolume* volume, voxel::RawVolume* selectionVolume, const glm::ivec3& pos) {
	const Selection& mode = selectionsArray[std::enum_value(_selectionType)];
	return mode.select.execute(volume, selectionVolume, pos);
}

}
