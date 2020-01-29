/**
 * @file
 */
#pragma once

#include "Compute.h"
#include "core/String.h"
#include <map>

#ifndef COMPUTE_POSTFIX
#define COMPUTE_POSTFIX ".cl"
#endif

namespace compute {

/**
 * @brief Compute shader. Base class for the generated shader wrappers.
 * @ingroup Compute
 */
class Shader {
protected:
	Id _program = InvalidId;

	bool _initialized = false;;
	mutable bool _active = false;

	typedef std::map<std::string, std::string> ShaderDefines;
	ShaderDefines _defines;

	core::String _name;

	core::String handlePragmas(const core::String& buffer) const;

	BufferFlag bufferFlags(const void* bufPtr, size_t size) const;

	template<class T>
	void* ptr(T& data) const {
		return const_cast<T*>(&data);
	}

	template<class T>
	void* ptr(const std::vector<T>& data) const {
		return const_cast<T*>(&data.front());
	}

	template<class T>
	void* ptr(std::vector<T>& data) const {
		return &data.front();
	}

public:
	virtual ~Shader();

	/**
	 * Some drivers don't support underscores in their defines...
	 */
	static core::String validPreprocessorName(const core::String& name);

	virtual void shutdown();

	bool loadFromFile(const core::String& filename);
	bool loadProgram(const core::String& filename);
	bool load(const core::String& name, const core::String& buffer);

	/**
	 * Use this to allocate memory for buffers that have the right size and alignment for
	 * possible zero-copy-buffers
	 * @param[in,out] size The size you need. Filled with the real buffer size after alignment.
	 * @sa bufferFree()
	 */
	void* bufferAlloc(size_t &size) const;
	/**
	 * You have to use this method to deallocate the buffer memory that you allocated with
	 * bufferAlloc()
	 * @sa bufferAlloc()
	 */
	void bufferFree(void *pointer) const;

	Id createKernel(const char *name);

	void deleteKernel(Id& kernel);

	core::String getSource(const core::String& buffer, bool finalize = true, std::vector<std::string>* includedFiles = nullptr) const;

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
	void addDefine(const core::String& name, const core::String& value);
};

inline bool Shader::isActive() const {
	return _active;
}

}
