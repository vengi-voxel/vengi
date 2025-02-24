add_library(meshoptimizer STATIC
	meshoptimizer.h
	allocator.cpp
	clusterizer.cpp
	indexcodec.cpp
	indexgenerator.cpp
	overdrawanalyzer.cpp
	overdrawoptimizer.cpp
	partition.cpp
	quantization.cpp
	simplifier.cpp
	spatialorder.cpp
	stripifier.cpp
	vcacheanalyzer.cpp
	vcacheoptimizer.cpp
	vertexcodec.cpp
	vertexfilter.cpp
	vfetchanalyzer.cpp
	vfetchoptimizer.cpp
)
target_include_directories(meshoptimizer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
engine_target_optimize(meshoptimizer)
