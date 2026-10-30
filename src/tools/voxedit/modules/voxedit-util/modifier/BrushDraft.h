/**
 * @file
 */
#pragma once

#include "DraftEditSession.h"
#include "brush/Brush.h"
#include "core/Function.h"

namespace voxedit {

/** Adjustable-operation lifetime owned by the host, independent of concrete tools. */
class BrushDraft {
public:
	virtual ~BrushDraft() = default;
	virtual bool active() const = 0;
	virtual bool enabled() const {
		return true;
	}
	virtual bool requiresSelection() const = 0;
	virtual bool valid(const scenegraph::SceneGraph &scene) const = 0;
	virtual BrushApplyResult refresh(scenegraph::SceneGraph &scene, const core::Buffer<core::UUID> &targets,
									 const voxel::Region &constraint, const BrushContext &input,
									 const BrushApplyOptions &options,
									 const core::Function<BrushApplyResult()> &finishPrevious) = 0;
	virtual BrushApplyResult apply(scenegraph::SceneGraph &scene) = 0;
	virtual void clear() = 0;
	virtual const scenegraph::SceneGraph *view() const = 0;
	virtual bool paletteChanged(const core::UUID &uuid) const = 0;
	virtual bool normalPaletteChanged(const core::UUID &uuid) const = 0;
	virtual voxel::Region dirtyRegion(const core::UUID &uuid) const = 0;
};

/** The only type erasure is at registration; settings and runtime remain typed. */
template<class Operation>
class TypedBrushDraft final : public BrushDraft {
public:
	using Parameters = typename Operation::Parameters;
	using Read = core::Function<Parameters(const BrushContext &)>;
	using Write = core::Function<void(const Parameters &)>;

private:
	DraftEditSession<Operation> _session;
	Read _read;
	Write _write;
	core::Function<void()> _reset;
	core::Function<bool()> _enabled;

public:
	TypedBrushDraft(const Read &read, const Write &write, const core::Function<void()> &reset,
					const core::Function<bool()> &enabled = {})
		: _read(read), _write(write), _reset(reset), _enabled(enabled) {
	}
	bool enabled() const override {
		return !_enabled || _enabled();
	}
	bool requiresSelection() const override {
		return Operation::requiresSelection;
	}
	bool active() const override {
		return _session.active();
	}
	bool valid(const scenegraph::SceneGraph &scene) const override {
		return _session.valid(scene);
	}
	BrushApplyResult refresh(scenegraph::SceneGraph &scene, const core::Buffer<core::UUID> &targets,
							 const voxel::Region &constraint, const BrushContext &input,
							 const BrushApplyOptions &options,
							 const core::Function<BrushApplyResult()> &finishPrevious) override {
		BrushApplyResult result;
		Operation validator;
		if (!validator.setParameters(_read(input))) {
			result.status = BrushApplyStatus::Failed;
			return result;
		}
		const Parameters parameters = validator.parameters();
		if (_session.active() && !Operation::sameOperation(_session.parameters(), parameters)) {
			result = finishPrevious();
			if (result.failed()) {
				return result;
			}
			_write(parameters);
		}
		if (!_session.active() && !_session.begin(scene, targets, constraint, parameters)) {
			result.status = BrushApplyStatus::InvalidTarget;
			return result;
		}
		if (!_session.valid(scene, true)) {
			result.status = BrushApplyStatus::InvalidTarget;
			return result;
		}
		_write(parameters);
		return _session.update(parameters, options);
	}
	BrushApplyResult apply(scenegraph::SceneGraph &scene) override {
		return _session.apply(scene);
	}
	void clear() override {
		_session.clear();
		_reset();
	}
	const scenegraph::SceneGraph *view() const override {
		return _session.view();
	}
	bool paletteChanged(const core::UUID &uuid) const override {
		return _session.paletteChanged(uuid);
	}
	bool normalPaletteChanged(const core::UUID &uuid) const override {
		return _session.normalPaletteChanged(uuid);
	}
	voxel::Region dirtyRegion(const core::UUID &uuid) const override {
		return _session.dirtyRegion(uuid);
	}
};

} // namespace voxedit
