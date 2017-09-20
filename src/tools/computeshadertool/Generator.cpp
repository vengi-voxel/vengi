/**
 * @file
 */

#include "Generator.h"
#include "core/String.h"
#include "core/Assert.h"
#include "Util.h"

namespace computeshadertool {

// TODO: doxygen
bool generateSrc(const io::FilesystemPtr& filesystem,
		const std::string& templateShader,
		const std::string& _name,
		const std::string& namespaceSrc,
		const std::string& shaderDirectory,
		const std::string& sourceDirectory,
		const std::vector<Kernel>& _kernels,
		const std::vector<Struct>& _structs) {
	std::string src(templateShader);
	std::string name = _name + "Shader";

	std::vector<std::string> shaderNameParts;
	core::string::splitString(name, shaderNameParts, "_");
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
	std::stringstream createKernels;
	std::stringstream kernels;
	for (const Kernel& k : _kernels) {
		kernels << "\n\tbool " << k.name << "(\n\t\t";
		bool first = true;
		for (const Parameter& p : k.parameters) {
			if (!first) {
				kernels << ",\n\t\t";
			}
			if (!p.qualifier.empty()) {
				kernels << p.qualifier << " ";
			}
			if (core::string::contains(p.type, "*")) {
				const util::CLTypeMapping& clType = util::vectorType(p.type);
				kernels << "/* " << p.type << "*/ std::vector<" << clType.type << ">& " << p.name;
			} else {
				const util::CLTypeMapping& clType = util::vectorType(p.type);
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
		kernels << ",\n\t\tconst glm::ivec" << k.workDimension << "& workSize\n\t) const {\n";
		for (size_t i = 0; i < k.parameters.size(); ++i) {
			const Parameter& p = k.parameters[i];
			if (core::string::contains(p.type, "*")) {
				const std::string& bufferName = core::string::format("_buffer_%s_%s", k.name.c_str(), p.name.c_str());
				const std::string size = "core::vectorSize(" + p.name + ")";
				kernels << "\t\tif (" << bufferName << " == InvalidId) {\n";
				kernels << "\t\t\tconst compute::BufferFlag flags = " << util::toString(p.flags) << " | bufferFlags(&" << p.name << "[0], " << size << ");\n";
				kernels << "\t\t\t" << bufferName << " = compute::createBufferFromType(flags, " << p.name << ");\n";
				kernels << "\t\t} else {\n";
				kernels << "\t\t\tcompute::updateBufferFromType(" << bufferName << ", " << p.name << ");\n";
				kernels << "\t\t}\n";
				kernels << "\t\tcompute::kernelArg(_kernel" << k.name << ", " << i << ", " << bufferName << ");\n";

				kernelMembers << "\tmutable compute::Id " << bufferName << " = compute::InvalidId;\n";

				shutdown << "\t\tcompute::deleteBuffer(" << bufferName << ");\n";
			} else {
				kernels << "\t\tcompute::kernelArg(_kernel" << k.name << ", ";
				kernels << i << ", ";
				kernels << p.name;
				kernels << ");\n";
			}
		}
		kernels << "\t\tglm::ivec3 globalWorkSize(0);\n";
		kernels << "\t\tfor (int i = 0; i < " << k.workDimension << "; ++i) {\n";
		kernels << "\t\t\tglobalWorkSize[i] += workSize[i];\n";
		kernels << "\t\t}\n";
		kernels << "\t\tconst bool state = compute::kernelRun(";
		kernels << "_kernel" << k.name << ", ";
		kernels << "globalWorkSize, ";
		kernels << k.workDimension;
		kernels << ");\n";
		for (size_t i = 0; i < k.parameters.size(); ++i) {
			const Parameter& p = k.parameters[i];
			if (!core::string::contains(p.type, "*")) {
				continue;
			}
			if ((p.flags & (compute::BufferFlag::ReadWrite | compute::BufferFlag::WriteOnly)) != compute::BufferFlag::None) {
				const std::string& bufferName = core::string::format("_buffer_%s_%s", k.name.c_str(), p.name.c_str());
				kernels << "#ifdef DEBUG\n";
				kernels << "\t\tcompute::finish();\n";
				kernels << "#endif\n";
				kernels << "\t\tif (state) {\n";
				kernels << "\t\t\tcore_assert_always(compute::readBufferIntoVector(" << bufferName << ", " << p.name << "));\n";
				kernels << "\t\t}\n";
			}
		}
		kernels << "\t\treturn state;\n";
		kernels << "\t}\n";

		kernelMembers << "\tcompute::Id _kernel" << k.name << " = compute::InvalidId;\n";

		core_assert_always(k.returnValue.type == "void");
		createKernels << "\t\t_kernel" << k.name << " = compute::createKernel(_program, \"" << k.name << "\");\n";

		shutdown << "\t\tcompute::deleteKernel(_kernel" << k.name << ");\n";
	}

	std::stringstream includes;

	std::stringstream structs;
	bool firstStruct = true;
	for (const Struct& s : _structs) {
		if (!firstStruct) {
			structs << "\n";
		}
		firstStruct = false;
		if (!s.comment.empty()) {
			structs << "/** " << s.comment << "*/\n";
		}
		structs << "\tstruct /*alignas(4)*/ " << s.name << " {\n";
		for (const Parameter& p : s.parameters) {
			const util::CLTypeMapping& clType = util::vectorType(p.type);
			if (!p.comment.empty()) {
				structs << "\t\t/** " << p.comment << "*/\n";
			}
			structs << "\t\t";
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
			structs << ";\n";
		}
		structs << "\t};\n";
	}

	src = core::string::replaceAll(src, "$name$", filename);
	src = core::string::replaceAll(src, "$namespace$", namespaceSrc);
	src = core::string::replaceAll(src, "$filename$", shaderDirectory + _name);
	src = core::string::replaceAll(src, "$kernels$", kernels.str());
	src = core::string::replaceAll(src, "$members$", kernelMembers.str());
	src = core::string::replaceAll(src, "$shutdown$", shutdown.str());
	src = core::string::replaceAll(src, "$structs$", structs.str());
	src = core::string::replaceAll(src, "$createkernels$", createKernels.str());
	src = core::string::replaceAll(src, "$includes$", includes.str());
	const std::string targetFile = sourceDirectory + filename + ".h";
	Log::info("Generate shader bindings for %s at %s", _name.c_str(), targetFile.c_str());
	if (!filesystem->syswrite(targetFile, src)) {
		Log::error("Failed to write %s", targetFile.c_str());
		return false;
	}
	return true;
}

}
