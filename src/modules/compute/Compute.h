/**
 * @file
 */
#pragma once

#include <string>
#include <cstring>
#include "Types.h"
#include "cl/CLTypes.h"
#include "core/Vector.h"

namespace compute {

bool init();
void shutdown();

bool updateBuffer(Id buffer, size_t size, const void* data, bool blockingWrite = true);
bool deleteBuffer(Id& buffer);
bool readBuffer(Id buffer, size_t size, void* data);
Id createBuffer(BufferFlag flags, size_t size = 0, void* data = nullptr);
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
