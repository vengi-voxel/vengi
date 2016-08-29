add_library(glslang-default-resource-limits
    ${CMAKE_CURRENT_SOURCE_DIR}/ResourceLimits.cpp
)
set_property(TARGET glslang-default-resource-limits PROPERTY FOLDER glslang)

target_include_directories(glslang-default-resource-limits
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${PROJECT_SOURCE_DIR}
)

set(SOURCES StandAlone.cpp)

set(LIBRARIES
    glslang
    OGLCompiler
    OSDependent
    HLSL
    SPIRV
    SPVRemapper
    glslang-default-resource-limits)

if(WIN32)
    set(LIBRARIES ${LIBRARIES} psapi)
elseif(UNIX)
    if(NOT ANDROID)
        set(LIBRARIES ${LIBRARIES} pthread)
    endif()
endif(WIN32)

engine_add_executable(TARGET glslangValidator SRCS ${SOURCES} WINDOWED)
glslang_set_link_args(glslangValidator)
target_link_libraries(glslangValidator ${LIBRARIES})
