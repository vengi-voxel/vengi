/**
 * @file
 */
#include "Shader.h"
#include "core/Var.h"
#include "core/Log.h"
#include "app/App.h"
#include "core/Assert.h"
#include "core/Var.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include "util/IncludeUtil.h"
#include "util/VarUtil.h"

namespace compute {

Shader::~Shader() {
	shutdown();
}

bool Shader::init() {
	if (!compute::supported()) {
		return false;
	}
	if (_initialized) {
		return true;
	}
	_initialized = true;
	return _initialized;
}

core::String Shader::handlePragmas(const core::String& buffer) const {
	// TODO: check the code for printf statements and activate the pragmas
	//#pragma OPENCL EXTENSION cl_amd_printf : enable
	//#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable
	return buffer;
}

void Shader::update(uint32_t deltaTime) {
	core_assert(_initialized);
}

bool Shader::activate() const {
	core_assert(_initialized);
	_active = true;
	return _active;
}

bool Shader::deactivate() const {
	if (!_active) {
		return false;
	}

	_active = false;
	return _active;
}

void Shader::shutdown() {
	_initialized = false;
	_active = false;
	compute::deleteProgram(_program);
}

bool Shader::load(const core::String& name, const core::String& buffer) {
	core_assert(_initialized);
	_name = name;
	Log::info("Load compute shader %s", name.c_str());
	const core::String& source = getSource(buffer);
	_program = compute::createProgram(source);
	if (_program == InvalidId) {
		return false;
	}
	return compute::configureProgram(_program);
}

// see https://software.intel.com/sites/default/files/managed/f1/25/opencl-zero-copy-in-opencl-1-2.pdf
BufferFlag Shader::bufferFlags(const void* bufPtr, size_t size) const {
	if ((uintptr_t) bufPtr % compute::requiredAlignment() != 0) {
		return BufferFlag::None;
	}
	if (size % 64 != 0) {
		return BufferFlag::None;
	}
	return BufferFlag::UseHostPointer;
}

void* Shader::bufferAlloc(size_t &size) const {
	const size_t alignment = compute::requiredAlignment();

	// round up to 64 bytes - according to intel opencl zero copy hints
	size = size + (~size + 1) % 64;

	core_assert(size >= sizeof(void*));
	core_assert(size / sizeof(void*) * sizeof(void*) == size);

	// allocate memory for the extra alignment and the pointer of the
	// position to free (the unaligned pointer)
	char* orig = new char[size + alignment + sizeof(void*)];
	char* aligned = orig + ((((size_t) orig + alignment + sizeof(void*)) & ~(alignment - 1)) - (size_t) orig);
	*((char**) aligned - 1) = orig;
	return aligned;
}

void Shader::bufferFree(void *pointer) const {
	if (pointer == nullptr) {
		return;
	}
	// the offset of the original (unaligned) pointer
	delete[] *((char**) pointer - 1);
}

Id Shader::createKernel(const char *name) {
	core_assert(_program != InvalidId);
	return compute::createKernel(_program, name);
}

void Shader::deleteKernel(Id& kernel) {
	core_assert(_initialized);
	compute::deleteKernel(kernel);
}

bool Shader::loadProgram(const core::String& filename) {
	return loadFromFile(filename + COMPUTE_POSTFIX);
}

bool Shader::loadFromFile(const core::String& filename) {
	const core::String& buffer = io::filesystem()->load(filename);
	if (buffer.empty()) {
		return false;
	}
	return load(filename, buffer);
}

core::String Shader::validPreprocessorName(const core::String& name) {
	return core::string::replaceAll(name, "_", "");
}

core::String Shader::getSource(const core::String& buffer, bool finalize, core::List<core::String>* includedFiles) const {
	if (buffer.empty()) {
		return "";
	}
	core::String src;

	util::visitVarSorted([&] (const core::VarPtr& var) {
		src.append("#define ");
		const core::String& validName = validPreprocessorName(var->name());
		src.append(validName);
		src.append(" ");
		core::String val;
		if (var->typeIsBool()) {
			val = var->boolVal() ? "1" : "0";
		} else {
			val = var->strVal();
		}
		src.append(val);
		src.append("\n");
	}, core::CV_SHADER);

	for (auto i = _defines.begin(); i != _defines.end(); ++i) {
		src.append("#ifndef ");
		src.append(i->key);
		src.append("\n");
		src.append("#define ");
		src.append(i->key);
		src.append(" ");
		src.append(i->value);
		src.append("\n");
		src.append("#endif\n");
	}

	core::List<core::String> includeDirs;
	includeDirs.insert(core::String(core::string::extractPath(_name)));
	const std::pair<core::String, bool>& includeFirst = util::handleIncludes(_name, buffer, includeDirs, includedFiles);
	src += includeFirst.first;
	int level = 0;
	while (core::string::contains(src, "#include")) {
		const std::pair<core::String, bool>& includeRecurse = util::handleIncludes(_name, src, includeDirs, includedFiles);
		src += includeRecurse.first;
		++level;
		if (level >= 10) {
			Log::warn("Abort shader include loop for %s", _name.c_str());
			break;
		}
	}

	src = handlePragmas(src);

	util::visitVarSorted([&] (const core::VarPtr& var) {
		const core::String& validName = validPreprocessorName(var->name());
		src = core::string::replaceAll(src, var->name(), validName);
	}, core::CV_SHADER);

	if (finalize) {
		// TODO: do placeholder/keyword replacement
	}
	return src;
}

void Shader::addDefine(const core::String& name, const core::String& value) {
	core_assert_msg(!_initialized, "Shader is already initialized");
	_defines.put(name, value);
}

}
