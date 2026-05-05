/**
 * @file
 */

#include "ShaderTool.h"
#include "Generator.h"
#include "Parser.h"
#include "app/I18N.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Process.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/Filesystem.h"
#include "util/IncludeUtil.h"
#include "video/Shader.h"
#include "voxel/SurfaceExtractor.h"

ShaderTool::ShaderTool(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "shadertool");
	_initialLogLevel = Log::Level::Warn;
	_saveConfiguration = false;
}

bool ShaderTool::parse(const core::String &filename, const core::String &buffer, bool vertex) {
	return shadertool::parse(io::filesystem()->sysAbsolutePath(filename), _shaderStruct, _shaderfile, buffer, vertex);
}

app::AppState ShaderTool::onConstruct() {
	registerArg("--glslang").setShort("-g").setDescription("Path to glslang validator binary");
	registerArg("--spirv").setDescription("Compile shaders to SPIR-V binary (requires glslang)");
	registerArg("--shader").setShort("-s").setDescription("The base name of the shader to create the c++ bindings for").addFlag(ARGUMENT_FLAG_MANDATORY);
	registerArg("--constantstemplate").setShort("-t").setDescription("The shader constants template file");
	registerArg("--buffertemplate").setShort("-b").setDescription("The uniform buffer template file").addFlag(ARGUMENT_FLAG_MANDATORY);
	registerArg("--namespace").setShort("-n").setDescription("Namespace to generate the source in").setDefaultValue("shader");
	registerArg("--shaderdir").setShort("-d").setDescription("Directory to load the shader from").setDefaultValue("shaders/");
	registerArg("--sourcedir").setDescription("Directory to generate the source in").addFlag(ARGUMENT_FLAG_MANDATORY);
	registerArg("-I").setDescription("Add additional include dir");
	registerArg("--printincludes").setDescription("Print the includes for the given shader");
	Log::trace("Set some shader config vars to let the validation work");

	const core::VarDef clientShadowMap(cfg::ClientShadowMap, true, "", "");
	core::Var::registerVar(clientShadowMap);

	return Super::onConstruct();
}

void ShaderTool::validate(const core::String &name) {
	if (_glslangValidatorBin.empty()) {
		return;
	}
	const core::String &writePath = filesystem()->homePath();
	core::DynamicArray<core::String> args;
	args.push_back(writePath + name);
	Log::debug("Execute glslang validator with the following commandline: %s %s", _glslangValidatorBin.c_str(),
			   args[0].c_str());
	io::BufferedReadWriteStream stream(4096);
	int exitCode = core::Process::exec(_glslangValidatorBin, args, nullptr, &stream);
	if (exitCode != 0) {
		Log::error("Failed to validate shader '%s'. Exitcode: %i", name.c_str(), exitCode);
		stream.seek(0);
		core::String output;
		stream.readString(stream.size(), output);
		Log::error("%s", output.c_str());
		Log::debug("%s %s%s", _glslangValidatorBin.c_str(), writePath.c_str(), name.c_str());
		_exitCode = exitCode;
	}
}

bool ShaderTool::compileSPIRV(const core::String& source, const core::String& shaderType, core::DynamicArray<uint32_t>& spirvBinary) {
	if (_glslangValidatorBin.empty()) {
		Log::error("glslang binary is required for SPIR-V compilation");
		return false;
	}

	// Inject layout(location=N) on bare in/out varyings that lack it.
	// Use variable name as key to ensure matching locations across stages.
	core::String processed;
	{
		core::DynamicArray<core::String> lines;
		core::string::splitString(source, lines, "\n");
		for (const core::String &line : lines) {
			core::String trimmed = core::string::trim(line);
			// Strip interpolation qualifiers for detection
			core::String check = trimmed;
			if (core::string::startsWith(check, "flat ")) {
				check = check.substr(5);
			} else if (core::string::startsWith(check, "smooth ")) {
				check = check.substr(7);
			} else if (core::string::startsWith(check, "noperspective ")) {
				check = check.substr(14);
			}
			// Match lines like "in type name;" or "out type name;" without layout
			if ((core::string::startsWith(check, "in ") || core::string::startsWith(check, "out ")) &&
				!core::string::contains(line, "layout") &&
				core::string::contains(trimmed, ";")) {
				// Extract variable name to get a consistent location from the map
				// Format: "[flat ]in/out type name;" - extract name
				core::String varName;
				size_t semi = check.find(";");
				if (semi != core::String::npos) {
					// find last space before semicolon
					size_t lastSpace = core::String::npos;
					for (size_t si = semi; si > 0; --si) {
						if (check[si - 1] == ' ') {
							lastSpace = si - 1;
							break;
						}
					}
					if (lastSpace != core::String::npos) {
						varName = check.substr(lastSpace + 1, semi - lastSpace - 1);
					}
				}
				int loc = -1;
				if (!varName.empty()) {
					auto it = _spirvVaryingLocations.find(varName);
					if (it != _spirvVaryingLocations.end()) {
						loc = it->second;
					} else {
						loc = _spirvNextVaryingLocation++;
						_spirvVaryingLocations.put(varName, loc);
					}
				} else {
					loc = _spirvNextVaryingLocation++;
				}
				processed += "layout(location = ";
				processed += core::string::toString(loc);
				processed += ") ";
				processed += line;
				processed += "\n";
			} else {
				processed += line;
				processed += "\n";
			}
		}
	}

	const core::String &writePath = filesystem()->homePath();
	const core::String tmpInput = writePath + "spirv_tmp" + shaderType;
	const core::String tmpOutput = tmpInput + ".spv";
	if (!io::Filesystem::sysWrite(tmpInput, processed)) {
		Log::error("Failed to write temp shader for SPIR-V compilation");
		return false;
	}
	core::DynamicArray<core::String> args;
	args.push_back("-V");
	args.push_back("--target-env");
	args.push_back("opengl");
	args.push_back("-o");
	args.push_back(tmpOutput);
	args.push_back(tmpInput);
	io::BufferedReadWriteStream stream(4096);
	int exitCode = core::Process::exec(_glslangValidatorBin, args, nullptr, &stream);
	if (exitCode != 0) {
		stream.seek(0);
		core::String output;
		stream.readString(stream.size(), output);
		Log::error("SPIR-V compilation failed for %s: %s", shaderType.c_str(), output.c_str());
		return false;
	}
	io::File spvFile(tmpOutput, io::FileMode::SysRead);
	spvFile.open(io::FileMode::SysRead);
	void *buf = nullptr;
	const int bytesRead = spvFile.read(&buf);
	if (bytesRead <= 0 || buf == nullptr) {
		Log::error("Failed to read SPIR-V output %s", tmpOutput.c_str());
		return false;
	}
	const size_t wordCount = (size_t)bytesRead / sizeof(uint32_t);
	spirvBinary.resize(wordCount);
	core_memcpy(spirvBinary.data(), buf, wordCount * sizeof(uint32_t));
	delete[] (uint8_t*)buf;
	return true;
}

bool ShaderTool::printInfo() {
	for (const auto &block : _shaderStruct.uniformBlocks) {
		Log::debug("Found uniform block %s with %i members", block.name.c_str(), int(block.members.size()));
	}
	for (const auto &v : _shaderStruct.uniforms) {
		Log::debug("Found uniform of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto &v : _shaderStruct.attributes) {
		Log::debug("Found attribute of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto &v : _shaderStruct.varyings) {
		Log::debug("Found varying of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto &v : _shaderStruct.outs) {
		Log::debug("Found out var of type %i with name %s", int(v.type), v.name.c_str());
	}

	const bool printIncludes = hasArg("--printincludes");
	if (printIncludes) {
		for (const core::String &i : _includes) {
			Log::info("%s%s", _shaderpath.c_str(), i.c_str());
		}
		return false;
	}
	return true;
}

core::Pair<core::String, bool> ShaderTool::getSource(const core::String &file) const {
	const io::FilesystemPtr &fs = filesystem();

	const core::Pair<core::String, bool> &retIncludes = util::handleIncludes(file, fs->load(file), _includeDirs);
	core::String src = retIncludes.first;
	int level = 0;
	bool success = retIncludes.second;
	while (core::string::contains(src, "#include")) {
		const core::Pair<core::String, bool> &ret = util::handleIncludes(file, src, _includeDirs);
		src = ret.first;
		success &= ret.second;
		++level;
		if (level >= 10) {
			Log::warn("Abort shader include loop for %s", file.c_str());
			break;
		}
	}
	return {src, success};
}

app::AppState ShaderTool::onRunning() {
	const core::String& shaderfile        = getArgVal("--shader");
	const bool printIncludes              = hasArg("--printincludes");
	if (!printIncludes) {
		_glslangValidatorBin              = getArgVal("--glslang");
		_spirv                            = hasArg("--spirv");
		_headerTemplateFile               = getArgVal("--headertemplate");
		_sourceTemplateFile               = getArgVal("--sourcetemplate");
		_uniformBufferTemplateFile        = getArgVal("--buffertemplate");
		_namespaceSrc                     = getArgVal("--namespace");
		_constantsTemplateFile            = getArgVal("--constantstemplate");
		_shaderDirectory                  = getArgVal("--shaderdir");
		_sourceDirectory                  = getArgVal("--sourcedir",
				core::string::path(_filesystem->basePath(), "src", "modules", _namespaceSrc));
		_postfix                          = getArgVal("--postfix", "");

		// handle include dirs
		_includeDirs.insert(".");
		int index = 0;
		for (;;) {
			const core::String& dir = getArgVal("-I", "", &index);
			if (dir.empty()) {
				break;
			}
			_includeDirs.insert(dir);
		}

		_shaderDirectory = core::string::sanitizeDirPath(_shaderDirectory);
		_sourceDirectory = core::string::sanitizeDirPath(_sourceDirectory);

		if (!_glslangValidatorBin.empty()) {
			Log::debug("Using glslangvalidator binary: %s", _glslangValidatorBin.c_str());
		}
		Log::debug("Using %s as output directory", _sourceDirectory.c_str());
		Log::debug("Using %s as namespace", _namespaceSrc.c_str());
		Log::debug("Using %s as shader directory", _shaderDirectory.c_str());
	}

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	_shaderfile = core::string::extractFilename(shaderfile.c_str());
	Log::debug("Preparing shader file %s", _shaderfile.c_str());
	const io::FilesystemPtr& fs = filesystem();
	_shaderpath = core::string::extractDir(shaderfile.c_str());
	const bool changedDir = fs->sysPushDir(core::Path(_shaderpath));

	video::Shader shader;

	const core::String& writePath = fs->homePath();
	Log::debug("Writing shader file %s to %s", _shaderfile.c_str(), writePath.c_str());

	const core::String& templateShaderHeader = fs->load(_headerTemplateFile);
	const core::String& templateShaderSource = fs->load(_sourceTemplateFile);
	const core::String& templateUniformBuffer = fs->load(_uniformBufferTemplateFile);
	const core::String& templateConstantsBuffer = _constantsTemplateFile.empty() ? "" : fs->load(_constantsTemplateFile);

	const core::String& computeFilename = _shaderfile + COMPUTE_POSTFIX;
	core::Pair<core::String, bool> computeBuffer = getSource(computeFilename);
	if (!computeBuffer.first.empty()) {
		if (!computeBuffer.second) {
			Log::error("Failed to parse compute shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return app::AppState::Cleanup;
		}
		const core::String& computeSrcSource = shader.getSource(video::ShaderType::Compute, computeBuffer.first, false, &_includes);
		if (!parse(computeFilename, computeSrcSource, false)) {
			Log::error("Failed to parse compute shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return app::AppState::Cleanup;
		}

		if (!printInfo()) {
			return app::AppState::Cleanup;
		}

		shadertool::SPIRVData spirvData;
		if (_spirv) {
			_spirvVaryingLocations.clear();
			_spirvNextVaryingLocation = 0;
			const core::String& computeSource = shader.getSource(video::ShaderType::Compute, computeBuffer.first, true);
			if (!compileSPIRV(computeSource, ".comp", spirvData.compute)) {
				Log::warn("SPIR-V compilation failed for compute shader %s, continuing without SPIR-V", _shaderfile.c_str());
				spirvData = shadertool::SPIRVData();
			}
		}

		if (!shadertool::generateSrc(templateShaderHeader, templateShaderSource, templateConstantsBuffer, templateUniformBuffer, _shaderStruct,
				filesystem(), _namespaceSrc, _sourceDirectory, _shaderDirectory, _postfix,
				"", "", "", computeBuffer.first, spirvData)) {
			Log::error("Failed to generate shader source for %s", _shaderfile.c_str());
			_exitCode = 1;
			return app::AppState::Cleanup;
		}

		const core::String& computeSource = shader.getSource(video::ShaderType::Compute, computeBuffer.first, true);
		const core::String& finalComputeFilename = _appname + "-" + computeFilename;
		fs->homeWrite(finalComputeFilename, computeSource);

		Log::debug("Validating shader file %s", _shaderfile.c_str());
		validate(finalComputeFilename);
		return app::AppState::Cleanup;
	}

	const core::String& fragmentFilename = _shaderfile + FRAGMENT_POSTFIX;
	const core::Pair<core::String, bool>& fragmentBuffer = getSource(fragmentFilename);
	if (fragmentBuffer.first.empty() || !fragmentBuffer.second) {
		Log::error("Could not load %s", fragmentFilename.c_str());
		_exitCode = 127;
		return app::AppState::Cleanup;
	}

	const core::String& vertexFilename = _shaderfile + VERTEX_POSTFIX;
	const core::Pair<core::String, bool>& vertexBuffer = getSource(vertexFilename);
	if (vertexBuffer.first.empty() || !vertexBuffer.second) {
		Log::error("Could not load %s", vertexFilename.c_str());
		_exitCode = 127;
		return app::AppState::Cleanup;
	}

	const core::String& geometryFilename = _shaderfile + GEOMETRY_POSTFIX;
	const core::Pair<core::String, bool>& geometryBuffer = getSource(geometryFilename);

	const core::String& fragmentSrcSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer.first, false, &_includes);
	const core::String& vertexSrcSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer.first, false, &_includes);

	if (!parse(fragmentFilename, fragmentSrcSource, false)) {
		Log::error("Failed to parse fragment shader %s", _shaderfile.c_str());
		_exitCode = 1;
		return app::AppState::Cleanup;
	}
	if (!parse(vertexFilename, vertexSrcSource, true)) {
		Log::error("Failed to parse vertex shader %s", _shaderfile.c_str());
		_exitCode = 1;
		return app::AppState::Cleanup;
	}
	if (!geometryBuffer.first.empty()) {
		if (!geometryBuffer.second) {
			Log::error("Failed to parse geometry shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return app::AppState::Cleanup;
		}
		const core::String& geometrySrcSource = shader.getSource(video::ShaderType::Geometry, geometryBuffer.first, false, &_includes);
		if (!parse(geometryFilename, geometrySrcSource, false)) {
			Log::error("Failed to parse geometry shader %s", _shaderfile.c_str());
			_exitCode = 1;
			return app::AppState::Cleanup;
		}
	}

	if (!printInfo()) {
		return app::AppState::Cleanup;
	}

	shadertool::SPIRVData spirvData;
	if (_spirv) {
		_spirvVaryingLocations.clear();
		_spirvNextVaryingLocation = 0;
		const core::String& vertSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer.first, true);
		const core::String& fragSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer.first, true);
		bool spirvOk = true;
		if (!compileSPIRV(vertSource, ".vert", spirvData.vertex)) {
			Log::warn("SPIR-V compilation failed for vertex shader %s", _shaderfile.c_str());
			spirvOk = false;
		}
		if (spirvOk && !compileSPIRV(fragSource, ".frag", spirvData.fragment)) {
			Log::warn("SPIR-V compilation failed for fragment shader %s", _shaderfile.c_str());
			spirvOk = false;
		}
		if (spirvOk && !geometryBuffer.first.empty()) {
			const core::String& geomSource = shader.getSource(video::ShaderType::Geometry, geometryBuffer.first, true);
			if (!compileSPIRV(geomSource, ".geom", spirvData.geometry)) {
				Log::warn("SPIR-V compilation failed for geometry shader %s", _shaderfile.c_str());
				spirvOk = false;
			}
		}
		if (!spirvOk) {
			spirvData = shadertool::SPIRVData();
		}
	}

	if (!shadertool::generateSrc(templateShaderHeader, templateShaderSource, templateConstantsBuffer, templateUniformBuffer,
			_shaderStruct, filesystem(), _namespaceSrc, _sourceDirectory, _shaderDirectory, _postfix,
			vertexBuffer.first, geometryBuffer.first, fragmentBuffer.first, computeBuffer.first, spirvData)) {
		Log::error("Failed to generate shader source for %s", _shaderfile.c_str());
		_exitCode = 1;
		return app::AppState::Cleanup;
	}
	const core::String& fragmentSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer.first, true);
	const core::String& vertexSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer.first, true);
	const core::String& geometrySource = shader.getSource(video::ShaderType::Geometry, geometryBuffer.first, true);

	if (changedDir) {
		fs->sysPopDir();
	}

	const core::String& finalFragmentFilename = _appname + "-" + fragmentFilename;
	const core::String& finalVertexFilename = _appname + "-" + vertexFilename;
	const core::String& finalGeometryFilename = _appname + "-" + geometryFilename;
	fs->homeWrite(finalFragmentFilename, fragmentSource);
	fs->homeWrite(finalVertexFilename, vertexSource);
	if (!geometrySource.empty()) {
		fs->homeWrite(finalGeometryFilename, geometrySource);
	}

	Log::debug("Validating shader file %s", _shaderfile.c_str());

	validate(finalFragmentFilename);
	validate(finalVertexFilename);
	if (!geometrySource.empty()) {
		validate(finalGeometryFilename);
	}

	return app::AppState::Cleanup;
}

CONSOLE_APP(ShaderTool)
