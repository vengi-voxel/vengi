/**
 * @file
 */

#pragma once

#include "core/Var.h"
#include "core/collection/DynamicArray.h"

namespace util {

template<class Functor>
static void visitVarSorted(Functor &&func, uint32_t flags) {
	core::DynamicArray<core::VarPtr> varList;
	core::Var::visit([&](const core::VarPtr &var) {
		if (flags == 0 || (var->getFlags() & flags) != 0) {
			varList.push_back(var);
		}
	});
	varList.sort([](const core::VarPtr &v1, const core::VarPtr &v2) { return v1->name() < v2->name(); });
	for (const core::VarPtr &var : varList) {
		func(var);
	}
}

class ScopedVarChange {
private:
	core::String _oldValue;
	core::VarPtr _var;

public:
	ScopedVarChange(const core::VarPtr &var, const core::String &value) {
		_var = var;
		_oldValue = _var->strVal();
		_var->setVal(value);
	}
	ScopedVarChange(const core::VarPtr &var, int value) {
		_var = var;
		_oldValue = _var->strVal();
		_var->setVal(value);
	}
	ScopedVarChange(const core::VarPtr &var, float value) {
		_var = var;
		_oldValue = _var->strVal();
		_var->setVal(value);
	}
	ScopedVarChange(const core::String &name, const core::String &value)
		: ScopedVarChange(core::getVar(name), value) {
	}
	ScopedVarChange(const core::String &name, int value)
		: ScopedVarChange(core::getVar(name), value) {
	}
	ScopedVarChange(const core::String &name, float value)
		: ScopedVarChange(core::getVar(name), value) {
	}

	~ScopedVarChange() {
		_var->setVal(_oldValue);
	}
};

} // namespace util
