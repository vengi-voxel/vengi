/**
 * @file
 */

#include "ComputeShaderTool.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "compute/Shader.h"
#include <stack>
#include <string>

namespace {

static bool isQualifier(const std::string& token) {
	return token == "const" || core::string::startsWith(token, "__");
}

static std::string getAlignment(const std::string& type) {
	// TODO: alignas
#if 0
	typedef int16_t         cl_short    __attribute__((aligned(2)));
	typedef uint16_t        cl_ushort   __attribute__((aligned(2)));
	typedef int32_t         cl_int      __attribute__((aligned(4)));
	typedef uint32_t        cl_uint     __attribute__((aligned(4)));
	typedef int64_t         cl_long     __attribute__((aligned(8)));
	typedef uint64_t        cl_ulong    __attribute__((aligned(8)));

	typedef uint16_t        cl_half     __attribute__((aligned(2)));
	typedef float           cl_float    __attribute__((aligned(4)));
	typedef double          cl_double   __attribute__((aligned(8)));
#endif
	return type;
}

static std::string convertType(const std::string& type, std::string& afterName) {
	char c;
	const size_t size = type.size();
	size_t i;
	for (i = 1; i < size; ++i) {
		c = type[size - i];
		if (c == ' ' || c == '*') {
			continue;
		}
		break;
	}
	afterName = "";
	if (c < '0' || c > '9') {
		return getAlignment(type);
	}
	const int n = c - '0';
	if (n > 0 && n < 10) {
		afterName.append("[");
		const char buf[] = {c, '\0'};
		afterName.append(buf);
		afterName.append("]");
		const std::string& sub = type.substr(0, size - i);
		return getAlignment(sub);
	}
	return getAlignment(type);
}

static std::string toString(compute::BufferFlag flagMask) {
	std::string str;

#define CHECK_MASK(mask) \
	if ((flagMask & compute::BufferFlag::mask) == compute::BufferFlag::mask) { \
		if (!str.empty()) \
			str += " | "; \
		str += "compute::BufferFlag::" CORE_STRINGIFY(mask); \
	}
	CHECK_MASK(ReadWrite)
	CHECK_MASK(WriteOnly)
	CHECK_MASK(ReadOnly)
	CHECK_MASK(UseHostPointer)
	CHECK_MASK(AllocHostPointer)
	CHECK_MASK(CopyHostPointer)
#undef CHECK_MASK
	if (str.empty()) {
		str += "compute::BufferFlag::None";
	}
	return str;
}

}

ComputeShaderTool::ComputeShaderTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		core::App(filesystem, eventBus, timeProvider, 0) {
	init(ORGANISATION, "computeshadertool");
}

ComputeShaderTool::~ComputeShaderTool() {
}

bool ComputeShaderTool::validate(Kernel& kernel) {
	bool error = false;
	// check mutual exclusive parameter flags
	for (const Parameter& p : kernel.parameters) {
		if ((p.flags & compute::BufferFlag::UseHostPointer) != compute::BufferFlag::None) {
			if ((p.flags & compute::BufferFlag::CopyHostPointer) != compute::BufferFlag::None) {
				Log::error("CopyHostPointer and UseHostPointer are mutually exclusive");
				error = true;
			}
			if ((p.flags & compute::BufferFlag::AllocHostPointer) != compute::BufferFlag::None) {
				Log::error("AllocHostPointer and UseHostPointer are mutually exclusive");
				error = true;
			}
		}
		if ((p.flags & compute::BufferFlag::ReadWrite) != compute::BufferFlag::None) {
			if ((p.flags & compute::BufferFlag::ReadOnly) != compute::BufferFlag::None) {
				Log::error("ReadWrite and ReadOnly are mutually exclusive");
				error = true;
			}
			if ((p.flags & compute::BufferFlag::WriteOnly) != compute::BufferFlag::None) {
				Log::error("ReadWrite and WriteOnly are mutually exclusive");
				error = true;
			}
		}
		if ((p.flags & compute::BufferFlag::ReadOnly) != compute::BufferFlag::None
		 && (p.flags & compute::BufferFlag::WriteOnly) != compute::BufferFlag::None) {
			if ((p.flags & compute::BufferFlag::WriteOnly) != compute::BufferFlag::None) {
				Log::error("ReadOnly and WriteOnly are mutually exclusive");
				error = true;
			}
		}
	}
	return !error;
}

void ComputeShaderTool::generateSrc() {
	const std::string& templateShader = core::App::getInstance()->filesystem()->load(_shaderTemplateFile);
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
	for (Kernel& k : _kernels) {
		kernels << "\n\tbool " << k.name << "(";
		bool first = true;
		for (Parameter& p : k.parameters) {
			if (!first) {
				kernels << ", ";
			}
			if (!p.qualifier.empty()) {
				kernels << p.qualifier << " ";
			}
			std::string arrayDefinition;
			kernels << convertType(p.type, arrayDefinition) << " " << p.name << arrayDefinition;
			if (core::string::contains(p.type, "*")) {
				kernels << ", size_t " << p.name << "Size";
			}
			first = false;
		}
		kernels << ", int workSize, int workDim) const {\n";
		kernels << "\t\tcore_assert(workSize % workDim == 0);\n";
		for (size_t i = 0; i < k.parameters.size(); ++i) {
			const Parameter& p = k.parameters[i];
			if (core::string::contains(p.type, "*")) {
				const std::string& bufferName = core::string::format("_buffer_%s_%s", k.name.c_str(), p.name.c_str());
				kernels << "\t\tif (" << bufferName << " == InvalidId) {\n\t\t\t" << bufferName;
				kernels << " = compute::createBuffer(" << toString(p.flags) << " | ";
				std::string arrayDefinition;
				const std::string ctype = convertType(p.type, arrayDefinition);

				std::string size;
				if (core::string::contains(p.type, "*")) {
					size = p.name + "Size";
				} else {
					size = "sizeof(" + p.name + ")";
				}
				kernels << "bufferFlags(" << p.name << ", " << size << "), " << size << ", ";
				if ((p.flags & compute::BufferFlag::ReadOnly) != compute::BufferFlag::None) {
					kernels << "const_cast<";
					kernels << ctype;
					if (!arrayDefinition.empty()) {
						kernels << "*";
					}
					kernels << ">(" << p.name << ")";
				} else {
					kernels << p.name;
				}
				kernels << ");\n\t\t} else {\n";
				kernels << "\t\t\tcompute::updateBuffer(" << bufferName << ", " << size << ", " << p.name << ");\n\t\t}\n";
				kernelMembers << "\tmutable compute::Id " << bufferName << " = compute::InvalidId;\n";
				shutdown << "\t\tcompute::deleteBuffer(" << bufferName << ");\n";
				kernels << "\t\tcompute::kernelArg(_kernel" << k.name << ", ";
				kernels << i << ", " << bufferName << ");\n";
			} else {
				kernels << "\t\tcompute::kernelArg(_kernel" << k.name << ", ";
				kernels << i << ", " << p.name << ");\n";
			}
		}
		kernels << "\t\tconst bool state = compute::kernelRun(_kernel" << k.name << ", workSize, workDim);\n";
		for (size_t i = 0; i < k.parameters.size(); ++i) {
			const Parameter& p = k.parameters[i];
			if (!core::string::contains(p.type, "*")) {
				continue;
			}
			if ((p.flags & (compute::BufferFlag::ReadWrite | compute::BufferFlag::WriteOnly)) != compute::BufferFlag::None) {
				const std::string& bufferName = core::string::format("_buffer_%s_%s", k.name.c_str(), p.name.c_str());
				kernels << "\t\tcompute::readBuffer(" << bufferName << ", " << p.name << "Size, " << p.name << ");\n";
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
	for (const Struct& s : _structs) {
		structs << "\tstruct " << s.name << " {\n";
		for (const Parameter& p : s.parameters) {
			std::string out;
			structs << "\t\t" << convertType(p.type, out) << " " << p.name << out << ";\n";
		}
		structs << "\t};\n\n";
	}

	src = core::string::replaceAll(src, "$name$", filename);
	src = core::string::replaceAll(src, "$namespace$", _namespaceSrc);
	src = core::string::replaceAll(src, "$filename$", _shaderDirectory + _name);
	src = core::string::replaceAll(src, "$kernels$", kernels.str());
	src = core::string::replaceAll(src, "$members$", kernelMembers.str());
	src = core::string::replaceAll(src, "$shutdown$", shutdown.str());
	src = core::string::replaceAll(src, "$structs$", structs.str());
	src = core::string::replaceAll(src, "$createkernels$", createKernels.str());
	src = core::string::replaceAll(src, "$includes$", includes.str());
	const std::string targetFile = _sourceDirectory + filename + ".h";
	Log::info("Generate shader bindings for %s at %s", _name.c_str(), targetFile.c_str());
	if (!core::App::getInstance()->filesystem()->syswrite(targetFile, src)) {
		Log::error("Failed to write %s", targetFile.c_str());
		_exitCode = 100;
		requestQuit();
	}
}

bool ComputeShaderTool::parseStruct(core::Tokenizer& tok) {
	if (!tok.hasNext()) {
		Log::error("Failed to parse struct - not enough tokens - expected name");
		return false;
	}
	Struct structVar;
	structVar.name = tok.next();
	if (!tok.hasNext()) {
		Log::error("Failed to parse struct - not enough tokens");
		return false;
	}
	if (tok.next() != "{") {
		while (tok.hasNext()) {
			const std::string& token = tok.next();
			if (token == ";") {
				_structs.push_back(structVar);
			} else {
				Log::error("Failed to parse struct - invalid token");
				return false;
			}
		}
	}
	int depth = 1;
	bool valid = false;
	Parameter param;
	std::string lastToken;
	while (tok.hasNext()) {
		const std::string& token = tok.next();
		if (token == "{") {
			++depth;
		} else if (token == "}") {
			--depth;
			if (depth <= 0) {
				valid = true;
				break;
			}
		} else if (token == ";") {
			param.name = lastToken;
			structVar.parameters.push_back(param);
			param = Parameter();
		} else {
			if (isQualifier(token)) {
				param.qualifier = token;
			} else if (tok.isNext(";")) {
				if (param.type.empty()) {
					param.type = token;
				} else {
					param.type.append(" ");
					param.type.append(token);
				}
			}
		}
		lastToken = token;
	}
	if (valid) {
		_structs.push_back(structVar);
	}
	return valid;
}

bool ComputeShaderTool::parseKernel(core::Tokenizer& tok) {
	std::stack<std::string> stack;
	std::string prev;
	int inAttribute = 0;
	while (tok.hasNext()) {
		std::string token = tok.next();
		if (token == "{") {
			break;
		}
		if (core::string::startsWith(token, "__attribute__")) {
			if (!tok.hasNext()) {
				continue;
			}
			if (tok.next() != "(") {
				tok.prev();
				continue;
			}
			++inAttribute;
			while (tok.hasNext()) {
				token = tok.next();
				if (token == "(") {
					++inAttribute;
				} else if (token == ")") {
					--inAttribute;
					if (inAttribute == 0) {
						break;
					}
				}
			}
		}

		stack.push(token);
	}

	if (stack.empty()) {
		Log::error("Could not identify any kernel");
		return false;
	}

	std::vector<std::string> parameterTokens;
	while (!stack.empty()) {
		const std::string token = stack.top();
		stack.pop();
		if (token == ")") {
			continue;
		}
		if (token == "(") {
			break;
		}
		parameterTokens.insert(parameterTokens.begin(), token);
	}

	if (stack.empty()) {
		Log::error("Expected to get a method name");
		return false;
	}

	Kernel kernel;
	kernel.name = stack.top();
	stack.pop();

	bool added = false;
	Parameter parameter;
	while (!parameterTokens.empty()) {
		const std::string token = parameterTokens.back();
		parameterTokens.pop_back();
		if (token.empty()) {
			continue;
		}
		if (token == ",") {
			core_assert(!parameter.name.empty());
			if (core::string::startsWith(parameter.name, "*")) {
				parameter.name = parameter.name.substr(1, parameter.name.size());
				parameter.type.append("*");
			}
			kernel.parameters.insert(kernel.parameters.begin(), parameter);
			parameter = Parameter();
			continue;
		}
		if (core::string::startsWith(token, "__")) {
			// The "__" prefix is not required before the qualifiers, but we will continue to use the
			// prefix in this text for consistency. If the qualifier is not specified, the variable
			// gets allocated to "__private", which is the default qualifier.
			// TODO: handle these: __global, __local, __private
			if (core::string::startsWith(&token[2], "constant")
			 || core::string::startsWith(&token[2], "read_only")) {
				parameter.flags |= compute::BufferFlag::ReadOnly;
			} else if (core::string::startsWith(&token[2], "write_only")) {
				parameter.flags |= compute::BufferFlag::WriteOnly;
			}
			continue;
		}

		added = true;
		if (parameter.name.empty()) {
			parameter.name = token;
		} else if (token == "const") {
			core_assert_msg(parameter.qualifier.empty(), "found %s, but already have %s",
					token.c_str(), parameter.qualifier.c_str());
			parameter.flags &= ~(compute::BufferFlag::ReadWrite | compute::BufferFlag::WriteOnly);
			parameter.flags |= compute::BufferFlag::ReadOnly;
			parameter.qualifier = token;
		} else {
			// TODO: opencl data types - image2d_t, sampler_t, float4
			if (!parameter.type.empty()) {
				parameter.type = token + " " + parameter.type;
			} else {
				parameter.type = token;
			}
		}
	}
	if (added) {
		if (core::string::startsWith(parameter.name, "*")) {
			parameter.name = parameter.name.substr(1, parameter.name.size());
			parameter.type.append("*");
		}
		core_assert(!parameter.name.empty());
		kernel.parameters.insert(kernel.parameters.begin(), parameter);
	}

	std::stack<std::string> returnTokens;
	while (!stack.empty()) {
		const std::string token = stack.top();
		stack.pop();
		returnTokens.push(token);
	}

	if (returnTokens.empty()) {
		Log::error("Could not find return values");
		return false;
	}

	while (!returnTokens.empty()) {
		const std::string token = returnTokens.top();
		returnTokens.pop();
		if (!kernel.returnValue.type.empty()) {
			kernel.returnValue.type.append(" ");
		}
		kernel.returnValue.type.append(token);
	}

	if (!validate(kernel)) {
		return false;
	}

	_kernels.push_back(kernel);

	return true;
}

bool ComputeShaderTool::parse(const std::string& buffer) {
	core::Tokenizer tok(buffer, " ", "{}(),;*");
	while (tok.hasNext()) {
		const std::string& token = tok.next();
		Log::trace("token: %s", token.c_str());
		if (token == "__kernel" || token == "kernel") {
			if (!parseKernel(tok)) {
				Log::warn("Could not parse kernel");
			}
		} else if (token == "struct") {
			if (!parseStruct(tok)) {
				Log::warn("Could not parse struct");
			}
		}
	}
	Log::info("Found %i kernels", (int)_kernels.size());
	return true;
}

core::AppState ComputeShaderTool::onRunning() {
	if (_argc < 3) {
		_exitCode = 1;
		Log::error("Usage: %s <shaderfile> <shadertemplate> <namespace> <shader-dir> <src-generator-dir>",
				_argv[0]);
		Log::error("shaderfile        - relative to data dir");
		Log::error("shadertemplate    - path to shader template file");
		Log::error("namespace         - the c++ namespace");
		Log::error("shader-dir        - the directory where the shaders are found in - relative to data dir");
		Log::error("src-generator-dir - the output dir for the generated c++ code");
		Log::error("%s", "");
		Log::error("Example call");
		Log::error("%s shaders/test src/tools/computeshadertool/ComputeShaderTemplate.h.in compute shaders/ outdir",
				_argv[0]);

		return core::AppState::Cleanup;
	}

	const std::string shaderfile          = _argv[1];
	_shaderTemplateFile                   = _argv[2];
	_namespaceSrc    = _argc >= 4 ?         _argv[3] : "compute";
	_shaderDirectory = _argc >= 5 ?         _argv[4] : "shaders/";
	_sourceDirectory = _argc >= 6 ?         _argv[5] : _filesystem->basePath() + "src/modules/" + _namespaceSrc + "/";

	Log::debug("Using %s as output directory", _sourceDirectory.c_str());
	Log::debug("Using %s as namespace", _namespaceSrc.c_str());
	Log::debug("Using %s as shader directory", _shaderDirectory.c_str());

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	const std::string computeFilename = shaderfile + COMPUTE_POSTFIX;
	const std::string computeBuffer = filesystem()->load(computeFilename);
	if (computeBuffer.empty()) {
		Log::error("Could not load %s", computeFilename.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	compute::Shader shader;
	const std::string& computeSrcSource = shader.getSource(computeBuffer, false);

	_name = std::string(core::string::extractFilename(shaderfile.c_str()));
	if (!parse(computeSrcSource)) {
		_exitCode = 1;
		return core::AppState::Cleanup;
	}
	generateSrc();

	const std::string& computeSource = shader.getSource(computeBuffer, true);

	Log::debug("Writing shader file %s to %s", shaderfile.c_str(), filesystem()->homePath().c_str());
	std::string finalComputeFilename = _appname + "-" + computeFilename;
	filesystem()->write(finalComputeFilename, computeSource);

	requestQuit();
	return core::AppState::Running;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	ComputeShaderTool app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
