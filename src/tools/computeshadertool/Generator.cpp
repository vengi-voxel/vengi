/**
 * @file
 */

#include "Generator.h"
#include "core/String.h"
#include "core/Assert.h"
#include "io/Filesystem.h"
#include "Util.h"
#include "Types.h"

namespace computeshadertool {

enum class BodyType {
	Pointer,
	Vector,
	Video,
	Native
};

static bool isBuffer(const std::string& str) {
	return core::string::contains(str, "*");
}

static std::string getBufferName(const Kernel& k, const Parameter& p) {
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

static void generateKernelDoxygen(const Kernel& k, std::stringstream& kernels, BodyType type) {
	kernels << "\t/**\n";
	kernels << "\t * @brief Kernel code for '" << k.name << "'\n";
	kernels << "\t * @return @c true if the execution was successful, @c false on error.\n";
	for (const Parameter& p : k.parameters) {
		if (!isBuffer(p.type)) {
			continue;
		}
		kernels << "\t * @param " << p.name;
		if (type == BodyType::Vector) {
			kernels << " vector with datatype that matches the CL type " << p.type;
		} else if (type == BodyType::Pointer) {
			kernels << " buffer that matches the CL type " << p.type;;
			kernels << "\n\t * @note The base pointer of this vector should be aligned (64 bytes) for optimal performance.\n";
		} else if (type == BodyType::Video) {
			kernels << " GL vbo";
		} else if (type == BodyType::Native) {
			kernels << " Native handle";
		}
	}
	kernels << "\t * @param[in] workSize Specify the number of global work-items per dimension (" << k.workDimension << ")\n";
	kernels << "\t * that will execute the kernel function\n";
	kernels << "\t */\n";
}

static void generateKernelHeader(const Kernel& k, std::stringstream& kernels, BodyType type) {
	kernels << "\tbool " << k.name << "(\n\t\t";
	bool first = true;
	for (const Parameter& p : k.parameters) {
		if (p.datatype == DataType::Sampler) {
			continue;
		}
		if (!first) {
			kernels << ",\n\t\t";
		}
		if (!p.qualifier.empty()) {
			kernels << p.qualifier << " ";
		}
		const util::CLTypeMapping& clType = util::vectorType(p.type);
		if (isBuffer(p.type)) {
			if (type == BodyType::Vector) {
				kernels << "std::vector<" << clType.type << ">& " << p.name;
			} else if (type == BodyType::Video) {
				kernels << "video::Buffer& " << p.name;
			} else if (type == BodyType::Native) {
				kernels << "compute::Id " << p.name;
			} else {
				kernels << clType.type << " ";
				if (isBuffer(clType.type) && !p.qualifier.empty()) {
					kernels << p.qualifier << " ";
				}
				kernels << "* " << p.name << ", size_t " << p.name << "Size";
			}
		} else {
			kernels << clType.type;
			if (p.byReference || (p.flags & compute::BufferFlag::ReadOnly) != compute::BufferFlag::None) {
				kernels << "&";
			}
			kernels << " " << p.name;
			if (clType.arraySize > 0) {
				kernels << "[" << clType.arraySize << "]";
			}
		}
		first = false;
	}
	kernels << ",\n\t\tconst glm::ivec" << k.workDimension << "& workSize\n\t) const";
}

static void generateKernelParameterTransfer(const Kernel& k, std::stringstream& kernels, BodyType type) {
	for (size_t i = 0; i < k.parameters.size(); ++i) {
		const Parameter& p = k.parameters[i];
		if (p.datatype == DataType::Sampler) {
			continue;
		}
		if (isBuffer(p.type)) {
			const std::string& bufferName = getBufferName(k, p);
			const util::CLTypeMapping& clType = util::vectorType(p.type);
			if (BodyType::Native == type) {
				kernels << "\t\tcompute::kernelArg(_kernel" << k.name << ", " << i << ", " << p.name<< ");\n";
			} else if (BodyType::Pointer == type) {
				kernels << "\t\tif (" << bufferName << " == InvalidId) {\n";
				kernels << "\t\t\tconst compute::BufferFlag flags = " << util::toString(p.flags);
				kernels << " | bufferFlags(" << p.name << ", " << p.name << "Size);\n";
				kernels << "\t\t\t" << bufferName << " = compute::createBuffer(flags, " << p.name << "Size, const_cast<" << clType.type << "*>(" << p.name << "));\n";
				kernels << "\t\t} else {\n";
				kernels << "\t\t\tcompute::updateBuffer(" << bufferName << ", " << p.name << "Size, " << p.name << ");\n";
				kernels << "\t\t}\n";
			} else if (BodyType::Video == type) {
				kernels << "\t\tif (" << bufferName << " == InvalidId) {\n";
				kernels << "\t\t\tconst compute::BufferFlag flags = " << util::toString(p.flags);
				kernels << ";\n";
				kernels << "\t\t\t" << bufferName << " = computevideo::createBuffer(flags, " << p.name << ");\n";
				kernels << "\t\t} else {\n";
				kernels << "\t\t\t// TODO\n";
				kernels << "\t\t}\n";
			}
		} else {
			if (BodyType::Native == type) {
				kernels << "\t\tcompute::kernelArg(_kernel" << k.name << ", ";
				kernels << i << ", ";
				kernels << p.name;
				if (i < k.parameters.size() - 1 && (p.datatype == DataType::Image2D || p.datatype == DataType::Image3D)) {
					if (k.parameters[i + 1].datatype == DataType::Sampler) {
						kernels << ", " << (i + 1);
					}
				}
				kernels << ");\n";
			}
		}
	}
}

static void generateKernelExecution(const Kernel& k, std::stringstream& kernels, BodyType type) {
	if (type == BodyType::Native) {
		kernels << "\t\tglm::ivec3 globalWorkSize(0);\n";
		kernels << "\t\tfor (int i = 0; i < " << k.workDimension << "; ++i) {\n";
		kernels << "\t\t\tglobalWorkSize[i] += workSize[i];\n";
		kernels << "\t\t}\n";

		kernels << "\t\tconst bool state = compute::kernelRun(";
		kernels << "_kernel" << k.name << ", ";
		kernels << "globalWorkSize, ";
		kernels << k.workDimension;
		kernels << ");\n";
		return;
	}

	if (type == BodyType::Video) {
		for (size_t i = 0; i < k.parameters.size(); ++i) {
			const Parameter& p = k.parameters[i];
			if (!isBuffer(p.type)) {
				continue;
			}
			const std::string& bufferName = getBufferName(k, p);
			kernels << "\t\tcomputevideo::enqueueAcquire(" << bufferName << ");\n";
		}
	}

	kernels << "\t\tconst bool state = " << k.name << "(";
	bool first = true;
	for (const Parameter& p : k.parameters) {
		if (p.datatype == DataType::Sampler) {
			continue;
		}
		if (!first) {
			kernels << ", ";
		}
		const util::CLTypeMapping& clType = util::vectorType(p.type);
		if (isBuffer(p.type)) {
			const util::CLTypeMapping& clType = util::vectorType(p.type);
			const std::string& bufferName = getBufferName(k, p);
			if (type == BodyType::Vector) {
				kernels << p.name << ".data(), core::vectorSize(" << p.name << ")";
			} else if (type == BodyType::Pointer) {
				kernels << bufferName;
			} else if (type == BodyType::Video) {
				kernels << bufferName;
			} else {
				kernels << p.name;
			}
		} else {
			kernels << p.name;
		}
		first = false;
	}
	kernels << ", workSize);\n";
}

static void generateKernelResultTransfer(const Kernel& k, std::stringstream& kernels, BodyType type) {
	if (BodyType::Native == type) {
		kernels << "\t\treturn state;\n";
		return;
	}
	for (size_t i = 0; i < k.parameters.size(); ++i) {
		const Parameter& p = k.parameters[i];
		if (!isBuffer(p.type)) {
			continue;
		}
		if ((p.flags & (compute::BufferFlag::ReadWrite | compute::BufferFlag::WriteOnly)) != compute::BufferFlag::None) {
			const std::string& bufferName = getBufferName(k, p);
			if (type == BodyType::Video) {
				kernels << "\t\tcomputevideo::enqueueRelease(" << bufferName << ");\n";
			} else if (BodyType::Pointer == type) {
				kernels << "\t\tif (state) {\n";
				kernels << "\t\t\tcore_assert_always(compute::readBuffer(" << bufferName << ", " << p.name << "Size, " << p.name << "));\n";
				kernels << "\t\t}\n";
			}
		}
	}
	kernels << "\t\treturn state;\n";
}

static void generateKernelBody(const Kernel& k, std::stringstream& kernels, BodyType type) {
	kernels << " {\n";
	generateKernelParameterTransfer(k, kernels, type);
	generateKernelExecution(k, kernels, type);
	generateKernelResultTransfer(k, kernels, type);
	kernels << "\t}\n";
}

static void generateKernelMembers(const Kernel& k, std::stringstream& kernelMembers, std::stringstream& shutdown) {
	for (size_t i = 0; i < k.parameters.size(); ++i) {
		const Parameter& p = k.parameters[i];
		if (!isBuffer(p.type)) {
			continue;
		}
		const std::string& bufferName = getBufferName(k, p);
		kernelMembers << "\t/**\n";
		kernelMembers << "\t * @brief Buffer for '" << p.name << "'\n";
		kernelMembers << "\t */\n";
		kernelMembers << "\tmutable compute::Id " << bufferName << " = compute::InvalidId;\n";
		shutdown << "\t\tcompute::deleteBuffer(" << bufferName << ");\n";
	}

	kernelMembers << "\tcompute::Id _kernel" << k.name << " = compute::InvalidId;\n";
}

static void generateStructs(const std::vector<Struct>& _structs, std::stringstream& structs) {
	bool firstStruct = true;
	for (const Struct& s : _structs) {
		if (!firstStruct) {
			structs << "\n";
		}
		firstStruct = false;
		if (!s.comment.empty()) {
			structs << "/** " << s.comment << "*/\n";
		}
		structs << "\t";
		if (s.isEnum) {
			structs << "enum ";
		} else {
			structs << "struct /*alignas(4)*/ ";
		}
		structs << s.name << " {\n";
		const size_t size = s.parameters.size();
		for (size_t i = 0; i < size; ++i) {
			const Parameter& p = s.parameters[i];
			if (!p.comment.empty()) {
				structs << "\t\t/** " << p.comment << "*/\n";
			}
			structs << "\t\t";
			if (s.isEnum) {
				structs << p.name;
				if (!p.value.empty()) {
					structs << " = " << p.value;
				}
			} else {
				const util::CLTypeMapping& clType = util::vectorType(p.type);
				const int alignment = util::alignment(clType.type);
				if (alignment > 1) {
					structs << "alignas(" << alignment << ") ";
				}
				structs << clType.type;
				structs << " /* '" << p.type << "' */ ";
				structs << p.name;
				if (clType.arraySize > 0) {
					structs << "[" << clType.arraySize << "]";
				}
			}
			if (s.isEnum) {
				if (i < size - 1) {
					structs << ",";
				}
				structs << "\n";
			} else {
				structs << ";\n";
			}
		}
		structs << "\t};\n";
	}
}

static void generateKernel(const Kernel& k, std::stringstream& kernels, BodyType type) {
	if (type == BodyType::Video) {
		kernels << "#ifdef COMPUTEVIDEO\n";
	}
	generateKernelDoxygen(k, kernels, type);
	generateKernelHeader(k, kernels, type);
	generateKernelBody(k, kernels, type);
	if (type == BodyType::Video) {
		kernels << "#endif";
	}
	kernels << "\n";
}

bool generateSrc(const io::FilesystemPtr& filesystem,
		const std::string& templateShader,
		const std::string& _name,
		const std::string& namespaceSrc,
		const std::string& shaderDirectory,
		const std::string& sourceDirectory,
		const std::vector<Kernel>& _kernels,
		const std::vector<Struct>& _structs,
		const std::map<std::string, std::string>& _constants,
		const std::string& postfix,
		const std::string& shaderBuffer) {
	const std::string name = _name + "Shader";

	std::vector<std::string> shaderNameParts;
	core::string::splitString(name, shaderNameParts, "_-");
	std::string filename = "";
	for (std::string n : shaderNameParts) {
		if (n.length() > 1 || shaderNameParts.size() < 2) {
			n[0] = SDL_toupper(n[0]);
			filename += n;
		}
	}
	if (filename.empty()) {
		filename = name;
	}
	std::stringstream kernelMembers;
	std::stringstream shutdown;
	for (const Kernel& k : _kernels) {
		generateKernelMembers(k, kernelMembers, shutdown);
	}

	std::stringstream createKernels;
	for (const Kernel& k : _kernels) {
		createKernels << "\t\t_kernel" << k.name << " = compute::createKernel(_program, \"" << k.name << "\");\n";
		shutdown << "\t\tcompute::deleteKernel(_kernel" << k.name << ");\n";
	}

	std::stringstream kernels;
	for (const Kernel& k : _kernels) {
		kernels << "\n";
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
		kernels << "\t/**\n";
		kernels << "\t * @brief Exported from shader code by @code $constant " << e.first << " " << e.second << " @endcode\n";
		kernels << "\t */\n";
		kernels << "\tinline static const char* get" << util::convertName(e.first, true) << "() {\n";
		kernels << "\t\treturn \"" << e.second << "\";\n";
		kernels << "\t}\n";
	}

	std::stringstream structs;
	generateStructs(_structs, structs);

	std::string src(templateShader);
	src = core::string::replaceAll(src, "$constant", "//");
	src = core::string::replaceAll(src, "$name$", filename);
	src = core::string::replaceAll(src, "$namespace$", namespaceSrc);
	src = core::string::replaceAll(src, "$filename$", shaderDirectory + _name);
	src = core::string::replaceAll(src, "$kernels$", kernels.str());
	src = core::string::replaceAll(src, "$members$", kernelMembers.str());
	src = core::string::replaceAll(src, "$shutdown$", shutdown.str());
	src = core::string::replaceAll(src, "$structs$", structs.str());
	src = core::string::replaceAll(src, "$createkernels$", createKernels.str());
	src = core::string::replaceAll(src, "$shaderbuffer$", shaderBuffer);
	const std::string targetFile = sourceDirectory + filename + ".h" + postfix;
	Log::info("Generate shader bindings for %s at %s", _name.c_str(), targetFile.c_str());
	if (!filesystem->syswrite(targetFile, src)) {
		Log::error("Failed to write %s", targetFile.c_str());
		return false;
	}
	return true;
}

}
