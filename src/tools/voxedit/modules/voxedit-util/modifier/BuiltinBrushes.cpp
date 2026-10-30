/**
 * @file
 */

#include "BrushDraft.h"
#include "BrushRegistry.h"
#include "brush/ExtrudeBrush.h"
#include "brush/LineBrush.h"
#include "brush/operation/LineOperation.h"
#include "brush/NormalBrush.h"
#include "brush/PaintBrush.h"
#include "brush/PlaneBrush.h"
#include "brush/RulerBrush.h"
#include "brush/SculptBrush.h"
#include "brush/SelectBrush.h"
#include "brush/ShapeBrush.h"
#include "brush/StampBrush.h"
#include "brush/TextBrush.h"
#include "brush/TextureBrush.h"
#include "brush/TransformBrush.h"

namespace voxedit {

class ColorPickerBrush final : public Brush {
protected:
	void generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &, const BrushContext &,
				  const voxel::Region &) override {
	}

public:
	ColorPickerBrush() : Brush(BrushType::None, ModifierType::ColorPicker, ModifierType::ColorPicker) {
	}
	bool readOnly() const override {
		return true;
	}
	voxel::Region calcRegion(const BrushContext &input) const override {
		return voxel::Region(input.cursorPosition, input.cursorPosition);
	}
};

void registerBuiltinBrushes(BrushRegistry &registry, SceneManager *sceneManager) {
	registry.add("picker", core::make_shared<ColorPickerBrush>());
	registry.add("shape", core::make_shared<ShapeBrush>());
	registry.add("plane", core::make_shared<PlaneBrush>());
	registry.add("stamp", core::make_shared<StampBrush>(sceneManager));
	auto line = core::make_shared<LineBrush>();
	auto lineDraft = core::make_shared<TypedBrushDraft<LineOperation>>(
		[line](const BrushContext &input) {
			LineParameters p;
			for (int i = 0; i < line->pendingSegmentCount(); ++i) {
				p.segments.push_back(*line->segment(i));
			}
			p.stipple = line->stipplePattern();
			p.thickness = line->thickness();
			p.voxel = input.cursorVoxel;
			return p;
		},
		[](const LineParameters &) {}, [line]() { line->resetRuntime(); }, [line]() { return line->bezier(); });
	registry.add("line", line, lineDraft);
	registry.add("paint", core::make_shared<PaintBrush>());
	registry.add("text", core::make_shared<TextBrush>());
	registry.add("select", core::make_shared<SelectBrush>(sceneManager));
	registry.add("texture", core::make_shared<TextureBrush>(sceneManager));
	registry.add("normal", core::make_shared<NormalBrush>());
	auto extrude = core::make_shared<ExtrudeBrush>();
	auto extrudeDraft = core::make_shared<TypedBrushDraft<ExtrudeOperation>>(
		[extrude](const BrushContext &input) {
			auto p = extrude->parameters();
			p.voxel = input.cursorVoxel;
			return p;
		},
		[extrude](const ExtrudeParameters &p) { extrude->setParameters(p); },
		[extrude]() { extrude->clearOperation(); });
	registry.add("extrude", extrude, extrudeDraft);
	auto transform = core::make_shared<TransformBrush>();
	auto transformDraft = core::make_shared<TypedBrushDraft<TransformOperation>>(
		[transform](const BrushContext &) { return transform->parameters(); },
		[transform](const TransformParameters &p) { transform->setParameters(p); },
		[transform]() { transform->commitCurrentTransform(); });
	registry.add("transform", transform, transformDraft);
	auto sculpt = core::make_shared<SculptBrush>();
	auto sculptDraft = core::make_shared<TypedBrushDraft<SculptOperation>>(
		[sculpt](const BrushContext &input) {
			auto p = sculpt->parameters();
			p.voxel = input.cursorVoxel;
			p.cursor = input.cursorPosition;
			return p;
		},
		[sculpt](const SculptParameters &p) { sculpt->acceptParameters(p); }, [sculpt]() { sculpt->clearOperation(); },
		[sculpt]() { return sculpt->sculptMode() != SculptMode::ExtendPlane; });
	registry.add("sculpt", sculpt, sculptDraft);
	registry.add("ruler", core::make_shared<RulerBrush>());
}

} // namespace voxedit
