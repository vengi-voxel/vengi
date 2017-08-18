/**
 * @file
 */

#include "ComputeShaderTool.h"
#include "core/App.h"
#include "core/Assert.h"
#include "io/Filesystem.h"
#include "compute/Shader.h"
#include "Util.h"
#include <stack>
#include <string>

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

// TODO: doxygen
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
		kernels << "\n\tbool " << k.name << "(\n\t\t";
		bool first = true;
		for (Parameter& p : k.parameters) {
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

const simplecpp::Token *ComputeShaderTool::parseStruct(const simplecpp::Token *tok) {
	tok = tok->next;
	if (!tok) {
		Log::error("%s:%i:%i: error: Failed to parse struct - not enough tokens - expected name",
				tok->location.file().c_str(), tok->location.line, tok->location.col);
		return tok;
	}
	Struct structVar;
	structVar.name = tok->str;
	tok = tok->next;
	if (!tok->next) {
		Log::error("%s:%i:%i: error: Failed to parse struct - not enough tokens",
				tok->location.file().c_str(), tok->location.line, tok->location.col);
		return tok;
	}
	if (tok->str != "{") {
		for (; tok; tok = tok->next) {
			const std::string& token = tok->str;
			if (token == ";") {
				_structs.push_back(structVar);
				tok = tok->next;
				break;
			} else {
				Log::error("%s:%i:%i: error: Failed to parse struct - invalid token: %s",
						tok->location.file().c_str(), tok->location.line, tok->location.col, token.c_str());
				return tok;
			}
		}
	}
	int depth = 1;
	bool valid = false;
	Parameter param;
	for (tok = tok->next; tok; tok = tok->next) {
		const std::string& token = tok->str;
		if (token == "{") {
			++depth;
		} else if (token == "}") {
			--depth;
			if (depth <= 0) {
				valid = true;
				break;
			}
		} else if (tok->next && (tok->next->str == ";" || tok->next->str == "[")) {
			param.name = token;
			if (tok->next->str == "[") {
				for (tok = tok->next; tok; tok = tok->next) {
					param.name.append(tok->str);
					if (tok->str == "]") {
						break;
					}
				}
			}
			structVar.parameters.push_back(param);
			param = Parameter();
			if (tok) {
				tok = tok->next;
			}
		} else {
			if (util::isQualifier(token)) {
				param.qualifier = token;
			} else {
				if (param.type.empty()) {
					param.type = token;
				} else {
					param.type.append(" ");
					param.type.append(token);
				}
			}
		}
	}
	if (valid) {
		_structs.push_back(structVar);
	}
	return tok;
}

const simplecpp::Token *ComputeShaderTool::parseKernel(const simplecpp::Token *tok) {
	if (!tok) {
		return tok;
	}
	std::stack<std::string> stack;
	std::string prev;
	int inAttribute = 0;
	for (tok = tok->next; tok; tok = tok->next) {
		std::string token = tok->str;
		if (token == "{") {
			break;
		}
		if (core::string::startsWith(token, "__attribute__")) {
			if (!tok->next) {
				continue;
			}
			tok = tok->next;
			if (tok->str != "(") {
				tok = tok->previous;
				continue;
			}
			++inAttribute;
			for (tok = tok->next; tok; tok = tok->next) {
				token = tok->str;
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
		return tok;
	}

	int kernelDimensions = 1;
	core_assert(tok->str == "{");
	for (tok = tok->next; tok; tok = tok->next) {
		const std::string& token = tok->str;
		if (token == "}") {
			break;
		}
		if (token == "get_global_id" || token == "get_global_size") {
			Log::debug("found %s", token.c_str());
			tok = tok->next;
			if (!tok) {
				Log::error("Expected (");
				return tok;
			}
			if (tok->str != "(") {
				Log::error("Expected ( - got %s", tok->str.c_str());
				return tok;
			}
			tok = tok->next;
			if (!tok) {
				Log::error("Expected number");
				return tok;
			}
			if (!tok->number) {
				Log::error("Expected number, got %s", tok->str.c_str());
				return tok;
			}
			const int dimension = (int)(tok->str[0] - '0') + 1;
			Log::debug("found dimension %i", dimension);
			kernelDimensions = std::max(kernelDimensions, dimension);
			tok = tok->next;
			if (!tok) {
				Log::error("Expected )");
				return tok;
			}
			if (tok->str != ")") {
				Log::error("Expected ) - got %s", tok->str.c_str());
				return tok;
			}
		}
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
		return tok;
	}

	Kernel kernel;
	kernel.name = stack.top();
	kernel.workDimension = kernelDimensions;
	Log::debug("found kernel %s with dimension %i", kernel.name.c_str(), kernel.workDimension);
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
				parameter.type.append(" *");
			}
			kernel.parameters.insert(kernel.parameters.begin(), parameter);
			parameter = Parameter();
			continue;
		}
		// TODO: __local size must be the size in bytes for the buffer that create
		// TODO: handle these: __global, __local, __private
		if (core::string::startsWith(token, "__")) {
			// The "__" prefix is not required before the qualifiers, but we will continue to use the
			// prefix in this text for consistency. If the qualifier is not specified, the variable
			// gets allocated to "__private", which is the default qualifier.
			if (core::string::startsWith(&token[2], "constant")
			 || core::string::startsWith(&token[2], "read_only")) {
				parameter.flags &= ~(compute::BufferFlag::ReadWrite);
				parameter.flags |= compute::BufferFlag::ReadOnly;
			} else if (core::string::startsWith(&token[2], "write_only")) {
				parameter.flags &= ~(compute::BufferFlag::ReadWrite);
				parameter.flags |= compute::BufferFlag::WriteOnly;
				parameter.byReference = true;
			}
			continue;
		} else {
			if (token == "constant" || token == "read_only") {
				parameter.flags &= ~(compute::BufferFlag::ReadWrite);
				parameter.flags |= compute::BufferFlag::ReadOnly;
				parameter.qualifier = "const";
				continue;
			} else if (token == "write_only") {
				parameter.flags &= ~(compute::BufferFlag::ReadWrite);
				parameter.flags |= compute::BufferFlag::WriteOnly;
				parameter.byReference = true;
				continue;
			}
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
			// TODO: opencl data types - image2d_t, sampler_t
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
			parameter.type.append(" *");
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
		return tok;
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
		return tok;
	}

	_kernels.push_back(kernel);

	return tok;
}

bool ComputeShaderTool::parse(const std::string& buffer) {
	simplecpp::DUI dui;
	simplecpp::OutputList outputList;
	std::vector<std::string> files;
	std::stringstream f(buffer);
	simplecpp::TokenList rawtokens(f, files, _computeFilename, &outputList);
	std::map<std::string, simplecpp::TokenList*> included = simplecpp::load(rawtokens, files, dui, &outputList);
	simplecpp::TokenList output(files);
	simplecpp::preprocess(output, rawtokens, files, included, dui, &outputList);

	simplecpp::Location loc(files);
	std::stringstream comment;
	for (const simplecpp::Token *tok = output.cfront(); tok; tok = tok->next) {
		const std::string& token = tok->str;
		if (tok->comment) {
			comment << token;
			continue;
		}
		if (token == "__kernel" || token == "kernel") {
			tok = parseKernel(tok);
		} else if (token == "struct") {
			tok = parseStruct(tok);
		}
		comment.clear();
	}
	Log::info("Found %i kernels", (int)_kernels.size());
	simplecpp::cleanup(included);
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
	_computeFilename = shaderfile + COMPUTE_POSTFIX;
	const std::string computeBuffer = filesystem()->load(_computeFilename);
	if (computeBuffer.empty()) {
		Log::error("Could not load %s", _computeFilename.c_str());
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
	std::string finalComputeFilename = _appname + "-" + _computeFilename;
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
