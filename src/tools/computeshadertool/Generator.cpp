/**
 * @file
 */

#include "Generator.h"
#include "core/StringUtil.h"
#include "core/Assert.h"
#include "core/io/Filesystem.h"
#include "core/collection/DynamicArray.h"
#include "Util.h"
#include "Types.h"
#include "core/Log.h"

namespace computeshadertool {

enum class BodyType {
	Pointer,
	Vector,
	Video,
	Native
};

/**
 * @brief MSVC doesn't like strings that exceeds a certain length. So we have to split them up here.
 * @sa https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2026?view=vs-2019
 */
static core::String maxStringLength(const core::String& input) {
#ifdef _MSC_VER
	if (input.size() > 10000) {
		Log::debug("Need to split the shader source string");
		return "R\"(" + core::string::replaceAll(input, "\n", ")\"\nR\"(") + ")\"";
	}
#endif
	return "R\"(" + input + ")\"";
}

static bool isBuffer(const core::String& str) {
	return core::string::contains(str, "*");
}

static core::String getBufferName(const Kernel& k, const Parameter& p) {
	return core::string::format("_buffer_%s_%s", k.name.c_str(), p.name.c_str());
}

static int getBuffers(const Kernel& k, compute::BufferFlag flags = compute::BufferFlag::None) {
	int buffers = 0;
	for (const Parameter& p : k.parameters) {
		if (!isBuffer(p.type)) {
			continue;
		}
		if (flags != compute::BufferFlag::None && (p.flags & flags) != compute::BufferFlag::None) {
			continue;
		}
		++buffers;
	}
	return buffers;
}

static void generateKernelDoxygen(const Kernel& k, core::String& kernels, BodyType type) {
	kernels += "\t/**\n";
	kernels += "\t * @brief Kernel code for '";
	kernels += k.name;
	kernels += "'\n";
	kernels += "\t * @return @c true if the execution was successful, @c false on error.\n";
	for (const Parameter& p : k.parameters) {
		if (!isBuffer(p.type)) {
			continue;
		}
		kernels += "\t * @param ";
		kernels += p.name;
		if (type == BodyType::Vector) {
			kernels += " vector with datatype that matches the CL type ";
			kernels += p.type;
		} else if (type == BodyType::Pointer) {
			kernels += " buffer that matches the CL type ";
			kernels += p.type;
			kernels += "\n\t * @note The base pointer of this vector should be aligned (64 bytes) for optimal performance.";
		} else if (type == BodyType::Video) {
			kernels += " GL vbo";
		} else if (type == BodyType::Native) {
			kernels += " Native handle";
		}
		kernels += "\n";
	}
	kernels += "\t * @param[in] workSize Specify the number of global work-items per dimension (";
	kernels += core::string::toString(k.workDimension);
	kernels += ")\n";
	kernels += "\t * that will execute the kernel function\n";
	kernels += "\t */\n";
}

static void generateKernelHeader(const Kernel& k, core::String& kernels, BodyType type) {
	kernels += "\tbool ";
	kernels += k.name;
	kernels += "(\n\t\t";
	bool first = true;
	for (const Parameter& p : k.parameters) {
		if (p.datatype == DataType::Sampler) {
			continue;
		}
		if (!first) {
			kernels += ",\n\t\t";
		}
		if (!p.qualifier.empty()) {
			kernels += p.qualifier;
			kernels += " ";
		}
		const util::CLTypeMapping& clType = util::vectorType(p.type);
		if (isBuffer(p.type)) {
			if (type == BodyType::Vector) {
				kernels += "std::vector<";
				kernels += clType.type;
				kernels += ">& ";
				kernels += p.name;
			} else if (type == BodyType::Video) {
				kernels += "video::Buffer& ";
				kernels += p.name;
			} else if (type == BodyType::Native) {
				kernels += "compute::Id ";
				kernels += p.name;
			} else {
				kernels += clType.type;
				kernels += " ";
				if (isBuffer(clType.type) && !p.qualifier.empty()) {
					kernels += p.qualifier;
					kernels += " ";
				}
				kernels += "* ";
				kernels += p.name;
				kernels += ", size_t ";
				kernels += p.name;
				kernels += "Size";
			}
		} else {
			kernels += clType.type;
			if (p.byReference || (p.flags & compute::BufferFlag::ReadOnly) != compute::BufferFlag::None) {
				kernels += "&";
			}
			kernels += " ";
			kernels += p.name;
			if (clType.arraySize > 0) {
				kernels += "[";
				kernels += core::string::toString(clType.arraySize);
				kernels += "]";
			}
		}
		first = false;
	}
	kernels += ",\n\t\tconst glm::ivec";
	kernels += core::string::toString(k.workDimension);
	kernels += "& workSize\n\t) const";
}

static void generateKernelParameterTransfer(const Kernel& k, core::String& kernels, BodyType type) {
	for (size_t i = 0; i < k.parameters.size(); ++i) {
		const Parameter& p = k.parameters[i];
		if (p.datatype == DataType::Sampler) {
			continue;
		}
		if (isBuffer(p.type)) {
			const core::String& bufferName = getBufferName(k, p);
			const util::CLTypeMapping& clType = util::vectorType(p.type);
			if (BodyType::Native == type) {
				kernels += "\t\tcompute::kernelArg(_kernel";
				kernels += k.name;
				kernels += ", ";
				kernels += core::string::toString(i);
				kernels += ", ";
				kernels += p.name;
				kernels += ");\n";
			} else if (BodyType::Pointer == type) {
				kernels += "\t\tif (";
				kernels += bufferName;
				kernels += " == InvalidId) {\n";
				kernels += "\t\t\tconst compute::BufferFlag flags = ";
				kernels += util::toString(p.flags);
				kernels += " | bufferFlags(";
				kernels += p.name;
				kernels += ", ";
				kernels += p.name;
				kernels += "Size);\n";
				kernels += "\t\t\t";
				kernels += bufferName;
				kernels += " = compute::createBuffer(flags, ";
				kernels += p.name;
				kernels += "Size, const_cast<";
				kernels += clType.type;
				kernels += "*>(";
				kernels += p.name;
				kernels += "));\n";
				kernels += "\t\t} else {\n";
				kernels += "\t\t\tcompute::updateBuffer(";
				kernels += bufferName;
				kernels += ", ";
				kernels += p.name;
				kernels += "Size, ";
				kernels += p.name;
				kernels += ");\n";
				kernels += "\t\t}\n";
			} else if (BodyType::Video == type) {
				kernels += "\t\tif (";
				kernels += bufferName;
				kernels += " == InvalidId) {\n";
				kernels += "\t\t\tconst compute::BufferFlag flags = ";
				kernels += util::toString(p.flags);
				kernels += ";\n";
				kernels += "\t\t\t";
				kernels += bufferName;
				kernels += " = computevideo::createBuffer(flags, ";
				kernels += p.name;
				kernels += ");\n";
				kernels += "\t\t} else {\n";
				kernels += "\t\t\t// TODO\n";
				kernels += "\t\t}\n";
			}
		} else {
			if (BodyType::Native == type) {
				kernels += "\t\tcompute::kernelArg(_kernel";
				kernels += k.name;
				kernels += ", ";
				kernels += core::string::toString(i);
				kernels += ", ";
				kernels += p.name;
				if (i < k.parameters.size() - 1 && (p.datatype == DataType::Image2D || p.datatype == DataType::Image3D)) {
					if (k.parameters[i + 1].datatype == DataType::Sampler) {
						kernels += ", ";
						kernels += core::string::toString(i + 1);
					}
				}
				kernels += ");\n";
			}
		}
	}
}

static void generateKernelExecution(const Kernel& k, core::String& kernels, BodyType type) {
	if (type == BodyType::Native) {
		kernels += "\t\tglm::ivec3 globalWorkSize(0);\n";
		kernels += "\t\tfor (int i = 0; i < ";
		kernels += core::string::toString(k.workDimension);
		kernels += "; ++i) {\n";
		kernels += "\t\t\tglobalWorkSize[i] += workSize[i];\n";
		kernels += "\t\t}\n";

		kernels += "\t\tconst bool state = compute::kernelRun(";
		kernels += "_kernel";
		kernels += k.name;
		kernels += ", ";
		kernels += "globalWorkSize, ";
		kernels += core::string::toString(k.workDimension);
		kernels += ");\n";
		return;
	}

	if (type == BodyType::Video) {
		for (size_t i = 0; i < k.parameters.size(); ++i) {
			const Parameter& p = k.parameters[i];
			if (!isBuffer(p.type)) {
				continue;
			}
			const core::String& bufferName = getBufferName(k, p);
			kernels += "\t\tcomputevideo::enqueueAcquire(";
			kernels += bufferName;
			kernels += ");\n";
		}
	}

	kernels += "\t\tconst bool state = ";
	kernels += k.name;
	kernels += "(";
	bool first = true;
	for (const Parameter& p : k.parameters) {
		if (p.datatype == DataType::Sampler) {
			continue;
		}
		if (!first) {
			kernels += ", ";
		}
		if (isBuffer(p.type)) {
			const core::String& bufferName = getBufferName(k, p);
			if (type == BodyType::Vector) {
				kernels += p.name;
				kernels += ".data(), core::vectorSize(";
				kernels += p.name;
				kernels += ")";
			} else if (type == BodyType::Pointer) {
				kernels += bufferName;
			} else if (type == BodyType::Video) {
				kernels += bufferName;
			} else {
				kernels += p.name;
			}
		} else {
			kernels += p.name;
		}
		first = false;
	}
	kernels += ", workSize);\n";
}

static void generateKernelResultTransfer(const Kernel& k, core::String& kernels, BodyType type) {
	if (BodyType::Native == type) {
		kernels += "\t\treturn state;\n";
		return;
	}
	for (size_t i = 0; i < k.parameters.size(); ++i) {
		const Parameter& p = k.parameters[i];
		if (!isBuffer(p.type)) {
			continue;
		}
		if ((p.flags & (compute::BufferFlag::ReadWrite | compute::BufferFlag::WriteOnly)) != compute::BufferFlag::None) {
			const core::String& bufferName = getBufferName(k, p);
			if (type == BodyType::Video) {
				kernels += "\t\tcomputevideo::enqueueRelease(";
				kernels += bufferName;
				kernels += ");\n";
			} else if (BodyType::Pointer == type) {
				kernels += "\t\tif (state) {\n";
				kernels += "\t\t\tcore_assert_always(compute::readBuffer(";
				kernels += bufferName;
				kernels += ", ";
				kernels += p.name;
				kernels += "Size, ";
				kernels += p.name;
				kernels += "));\n";
				kernels += "\t\t}\n";
			}
		}
	}
	kernels += "\t\treturn state;\n";
}

static void generateKernelBody(const Kernel& k, core::String& kernels, BodyType type) {
	kernels += " {\n";
	generateKernelParameterTransfer(k, kernels, type);
	generateKernelExecution(k, kernels, type);
	generateKernelResultTransfer(k, kernels, type);
	kernels += "\t}\n";
}

static void generateKernelMembers(const Kernel& k, core::String& kernelMembers, core::String& shutdown) {
	for (size_t i = 0; i < k.parameters.size(); ++i) {
		const Parameter& p = k.parameters[i];
		if (!isBuffer(p.type)) {
			continue;
		}
		const core::String& bufferName = getBufferName(k, p);
		kernelMembers += "\t/**\n";
		kernelMembers += "\t * @brief Buffer for '";
		kernelMembers += p.name;
		kernelMembers += "'\n";
		kernelMembers += "\t */\n";
		kernelMembers += "\tmutable compute::Id ";
		kernelMembers += bufferName;
		kernelMembers += " = compute::InvalidId;\n";
		shutdown += "\t\tcompute::deleteBuffer(";
		shutdown += bufferName;
		shutdown += ");\n";
	}

	kernelMembers += "\tcompute::Id _kernel";
	kernelMembers += k.name;
	kernelMembers += " = compute::InvalidId;\n";
}

static void generateStructs(const core::List<Struct>& _structs, core::String& structs) {
	bool firstStruct = true;
	for (const Struct& s : _structs) {
		if (!firstStruct) {
			structs += "\n";
		}
		firstStruct = false;
		if (!s.comment.empty()) {
			structs += "/** ";
			structs += s.comment;
			structs += "*/\n";
		}
		structs += "\t";
		if (s.isEnum) {
			structs += "enum ";
		} else {
			structs += "struct /*alignas(4)*/ ";
		}
		structs += s.name;
		structs += " {\n";
		const size_t size = s.parameters.size();
		for (size_t i = 0; i < size; ++i) {
			const Parameter& p = s.parameters[i];
			if (!p.comment.empty()) {
				structs += "\t\t/** ";
				structs += p.comment;
				structs += "*/\n";
			}
			structs += "\t\t";
			if (s.isEnum) {
				structs += p.name;
				if (!p.value.empty()) {
					structs += " = ";
					structs += p.value;
				}
			} else {
				const util::CLTypeMapping& clType = util::vectorType(p.type);
				const int alignment = util::alignment(clType.type);
				if (alignment > 1) {
					structs += "alignas(";
					structs += alignment;
					structs += ") ";
				}
				structs += clType.type;
				structs += " /* '";
				structs += p.type;
				structs += "' */ ";
				structs += p.name;
				if (clType.arraySize > 0) {
					structs += "[";
					structs += clType.arraySize;
					structs += "]";
				}
			}
			if (s.isEnum) {
				if (i < size - 1) {
					structs += ",";
				}
				structs += "\n";
			} else {
				structs += ";\n";
			}
		}
		structs += "\t};\n";
	}
}

static void generateKernel(const Kernel& k, core::String& kernels, BodyType type) {
	if (type == BodyType::Video) {
		kernels += "#ifdef COMPUTEVIDEO\n";
	}
	generateKernelDoxygen(k, kernels, type);
	generateKernelHeader(k, kernels, type);
	generateKernelBody(k, kernels, type);
	if (type == BodyType::Video) {
		kernels += "#endif";
	}
	kernels += "\n";
}

bool generateSrc(const io::FilesystemPtr& filesystem,
		const core::String& templateShader,
		const core::String& _name,
		const core::String& namespaceSrc,
		const core::String& shaderDirectory,
		const core::String& sourceDirectory,
		const core::List<Kernel>& _kernels,
		const core::List<Struct>& _structs,
		const core::StringMap<core::String>& _constants,
		const core::String& postfix,
		const core::String& shaderBuffer) {
	const core::String name = _name + "Shader";

	core::DynamicArray<core::String> shaderNameParts;
	core::string::splitString(name, shaderNameParts, "_-");
	core::String filename = "";
	for (core::String n : shaderNameParts) {
		if (n.size() > 1 || shaderNameParts.size() < 2) {
			n[0] = SDL_toupper(n[0]);
			filename += n;
		}
	}
	if (filename.empty()) {
		filename = name;
	}
	core::String kernelMembers;
	core::String shutdown;
	for (const Kernel& k : _kernels) {
		generateKernelMembers(k, kernelMembers, shutdown);
	}

	core::String createKernels;
	for (const Kernel& k : _kernels) {
		createKernels += "\t\t_kernel";
		createKernels += k.name;
		createKernels += " = compute::createKernel(_program, \"";
		createKernels += k.name;
		createKernels += "\");\n";
		shutdown += "\t\tcompute::deleteKernel(_kernel";
		shutdown += k.name;
		shutdown += ");\n";
	}

	core::String kernels;
	for (const Kernel& k : _kernels) {
		kernels += "\n";
		generateKernel(k, kernels, BodyType::Native);
		const int buffers = getBuffers(k, compute::BufferFlag::ReadOnly);
		if (buffers > 0) {
			generateKernel(k, kernels, BodyType::Pointer);
			generateKernel(k, kernels, BodyType::Vector);
			generateKernel(k, kernels, BodyType::Video);
		}

		if (k.returnValue.type != "void") {
			Log::error("return value must be void (Kernel: %s)", k.name.c_str());
			return false;
		}
	}

	for (const auto& e : _constants) {
		kernels += "\t/**\n";
		kernels += "\t * @brief Exported from shader code by @code $constant ";
		kernels += e->key;
		kernels += " ";
		kernels += e->value;
		kernels += " @endcode\n";
		kernels += "\t */\n";
		if (core::string::isInteger(e->value)) {
			kernels += "\tinline static constexpr int get";
			kernels += util::convertName(e->key, true);
			kernels += "() {\n";
			kernels += "\t\treturn ";
			kernels += e->value;
			kernels += ";\n";
			kernels += "\t}\n";
		} else if (core::string::isNumber(e->value)) {
			kernels += "\tinline static constexpr double get";
			kernels += util::convertName(e->key, true);
			kernels += "() {\n";
			kernels += "\t\treturn ";
			kernels += e->value;
			kernels += ";\n";
			kernels += "\t}\n";
		} else {
			kernels += "\tinline static constexpr const char* get";
			kernels += util::convertName(e->key, true);
			kernels += "() {\n";
			kernels += "\t\treturn \"";
			kernels += e->value;
			kernels += "\";\n";
			kernels += "\t}\n";
		}
	}

	core::String structs;
	generateStructs(_structs, structs);

	core::String src(templateShader);
	src = core::string::replaceAll(src, "$constant", "//");
	src = core::string::replaceAll(src, "$name$", filename);
	src = core::string::replaceAll(src, "$namespace$", namespaceSrc);
	src = core::string::replaceAll(src, "$filename$", shaderDirectory + _name);
	src = core::string::replaceAll(src, "$kernels$", kernels);
	src = core::string::replaceAll(src, "$members$", kernelMembers);
	src = core::string::replaceAll(src, "$shutdown$", shutdown);
	src = core::string::replaceAll(src, "$structs$", structs);
	src = core::string::replaceAll(src, "$createkernels$", createKernels);
	src = core::string::replaceAll(src, "$shaderbuffer$", maxStringLength(shaderBuffer));
	const core::String targetFile = sourceDirectory + filename + ".h" + postfix;
	Log::info("Generate shader bindings for %s at %s", _name.c_str(), targetFile.c_str());
	if (!filesystem->syswrite(targetFile, src)) {
		Log::error("Failed to write %s", targetFile.c_str());
		return false;
	}
	return true;
}

}
