/**
 * @file
 */
#pragma once

#include <string>
#include <cstring>
#include <vector>
#include "Types.h"
#include "cl/CLTypes.h"
#include "core/Vector.h"

namespace compute {

bool init();
void shutdown();

Id createBuffer(BufferFlag flags, size_t size = 0, void* data = nullptr);
bool updateBuffer(Id buffer, size_t size, const void* data, bool blockingWrite = true);
bool deleteBuffer(Id& buffer);
bool readBuffer(Id buffer, size_t size, void* data);

namespace priv {

template<typename T> struct is_vector: public std::false_type {
};

template<typename T, typename A>
struct is_vector<std::vector<T, A>> : public std::true_type {
};

template<class T>
Id createBufferFromType(std::true_type, BufferFlag flags, T& data) {
	return createBuffer(flags, sizeof(T), const_cast<T*>(&data));
}

template<class T>
Id createBufferFromType(std::false_type, BufferFlag flags, const std::vector<T>& data) {
	if ((flags & BufferFlag::ReadOnly) != BufferFlag::None) {
		return createBuffer(flags, core::vectorSize(data), const_cast<T*>(&data.front()));
	}
	return createBuffer(flags, core::vectorCapacity(data), const_cast<T*>(&data.front()));
}

template<class T>
bool updateBufferFromType(std::true_type, Id buffer, T& data, bool blockingWrite) {
	return updateBuffer(buffer, sizeof(T), const_cast<T*>(&data), blockingWrite);
}

template<class T>
bool updateBufferFromType(std::false_type, Id buffer, const std::vector<T>& data, bool blockingWrite) {
	return updateBuffer(buffer, core::vectorSize(data), const_cast<T*>(&data.front()), blockingWrite);
}

}

template<class T>
bool updateBufferFromType(Id buffer, T& data, bool blockingWrite = true) {
	return updateBufferFromType(priv::is_vector<T>(), buffer, data, blockingWrite);
}

template<class T>
Id createBufferFromType(BufferFlag flags, T& data) {
	return createBufferFromType(priv::is_vector<T>(), flags, data);
}

template<class T>
bool readBufferIntoVector(Id buffer, std::vector<T>& data) {
	return readBuffer(buffer, core::vectorCapacity(data), (T*)&data.front());
}

Id createProgram(const std::string& source);
size_t requiredAlignment();
bool configureProgram(Id program);
bool deleteProgram(Id& program);

Id createKernel(Id program, const char *name);
bool deleteKernel(Id& kernel);
bool kernelArg(Id kernel, uint32_t index, size_t size, const void* data);
bool kernelRun(Id kernel, int workSize, int workDim = -1, bool blocking = true);

template<class T>
inline bool kernelArg(Id kernel, uint32_t index, T& t) {
	return kernelArg(kernel, index, sizeof(T), (const void*)&t);
}

template<>
inline bool kernelArg(Id kernel, uint32_t index, Id& t) {
	return kernelArg(kernel, index, sizeof(Id), (const void*)&t);
}

}
