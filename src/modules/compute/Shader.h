/**
 * @file
 */
#pragma once

#include "Compute.h"
#include <string>
#include <map>

#ifndef COMPUTE_POSTFIX
#define COMPUTE_POSTFIX ".cl"
#endif

namespace compute {

class Shader {
protected:
	Id _program = InvalidId;

	bool _initialized = false;;
	mutable bool _active = false;

	typedef std::map<std::string, std::string> ShaderDefines;
	ShaderDefines _defines;

	std::string _name;

	std::string handlePragmas(const std::string& buffer) const;
	std::string handleIncludes(const std::string& buffer) const;

	BufferFlag bufferFlags(const void* bufPtr, size_t size) const;
	void* bufferAlloc(size_t &size) const;
	void bufferFree(void *pointer) const;

public:
	virtual ~Shader();

	/**
	 * Some drivers don't support underscores in their defines...
	 */
	static std::string validPreprocessorName(const std::string& name);

	virtual void shutdown();

	bool loadFromFile(const std::string& filename);
	bool loadProgram(const std::string& filename);
	bool load(const std::string& name, const std::string& buffer);

	Id createKernel(const char *name);

	void deleteKernel(Id& kernel);

	std::string getSource(const std::string& buffer, bool finalize = true) const;

	/**
	 * If the shaders were loaded manually via @c ::load, then you have to initialize the shader manually, too
	 */
	bool init();

	virtual bool setup() {
		return false;
	}
	/**
	 * @brief Ticks the shader
	 */
	virtual void update(uint32_t deltaTime);

	/**
	 * @brief Bind the shader program
	 *
	 * @return @c true if is is useable now, @c false if not
	 */
	virtual bool activate() const;

	virtual bool deactivate() const;

	bool isActive() const;

	/**
	 * @brief Adds a new define in the form '#define value' to the shader source code
	 */
	void addDefine(const std::string& name, const std::string& value);
};

inline bool Shader::isActive() const {
	return _active;
}

}
