add_library(meshoptimizer STATIC
	meshoptimizer.h
	allocator.cpp
	clusterizer.cpp
	indexanalyzer.cpp
	indexcodec.cpp
	indexgenerator.cpp
	overdrawoptimizer.cpp
	partition.cpp
	quantization.cpp
	rasterizer.cpp
	simplifier.cpp
	spatialorder.cpp
	stripifier.cpp
	vcacheoptimizer.cpp
	vertexcodec.cpp
	vertexfilter.cpp
	vfetchoptimizer.cpp
)
target_include_directories(meshoptimizer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
engine_target_optimize(meshoptimizer)
