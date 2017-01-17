/**
 * @file
 */

#include "GLFunc.h"
#include <stdio.h>

#include <SDL_video.h>

#ifdef __cplusplus
extern "C" {
#endif

void GLLoadFunctions(void) {
	/* GL_VERSION_1_2 */

	glpfDrawRangeElements = (PFNGLDRAWRANGEELEMENTS_PROC*) SDL_GL_GetProcAddress("glDrawRangeElements");
	glpfTexImage3D = (PFNGLTEXIMAGE3D_PROC*) SDL_GL_GetProcAddress("glTexImage3D");
	glpfTexSubImage3D = (PFNGLTEXSUBIMAGE3D_PROC*) SDL_GL_GetProcAddress("glTexSubImage3D");
	glpfCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3D_PROC*) SDL_GL_GetProcAddress("glCopyTexSubImage3D");

	/* GL_VERSION_1_3 */

	glpfActiveTexture = (PFNGLACTIVETEXTURE_PROC*) SDL_GL_GetProcAddress("glActiveTexture");
	glpfSampleCoverage = (PFNGLSAMPLECOVERAGE_PROC*) SDL_GL_GetProcAddress("glSampleCoverage");
	glpfCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3D_PROC*) SDL_GL_GetProcAddress("glCompressedTexImage3D");
	glpfCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2D_PROC*) SDL_GL_GetProcAddress("glCompressedTexImage2D");
	glpfCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1D_PROC*) SDL_GL_GetProcAddress("glCompressedTexImage1D");
	glpfCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3D_PROC*) SDL_GL_GetProcAddress("glCompressedTexSubImage3D");
	glpfCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2D_PROC*) SDL_GL_GetProcAddress("glCompressedTexSubImage2D");
	glpfCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1D_PROC*) SDL_GL_GetProcAddress("glCompressedTexSubImage1D");
	glpfGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGE_PROC*) SDL_GL_GetProcAddress("glGetCompressedTexImage");

	/* GL_VERSION_1_4 */

	glpfBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATE_PROC*) SDL_GL_GetProcAddress("glBlendFuncSeparate");
	glpfMultiDrawArrays = (PFNGLMULTIDRAWARRAYS_PROC*) SDL_GL_GetProcAddress("glMultiDrawArrays");
	glpfMultiDrawElements = (PFNGLMULTIDRAWELEMENTS_PROC*) SDL_GL_GetProcAddress("glMultiDrawElements");
	glpfPointParameterf = (PFNGLPOINTPARAMETERF_PROC*) SDL_GL_GetProcAddress("glPointParameterf");
	glpfPointParameterfv = (PFNGLPOINTPARAMETERFV_PROC*) SDL_GL_GetProcAddress("glPointParameterfv");
	glpfPointParameteri = (PFNGLPOINTPARAMETERI_PROC*) SDL_GL_GetProcAddress("glPointParameteri");
	glpfPointParameteriv = (PFNGLPOINTPARAMETERIV_PROC*) SDL_GL_GetProcAddress("glPointParameteriv");
	glpfBlendColor = (PFNGLBLENDCOLOR_PROC*) SDL_GL_GetProcAddress("glBlendColor");
	glpfBlendEquation = (PFNGLBLENDEQUATION_PROC*) SDL_GL_GetProcAddress("glBlendEquation");

	/* GL_VERSION_1_5 */

	glpfGenQueries = (PFNGLGENQUERIES_PROC*) SDL_GL_GetProcAddress("glGenQueries");
	glpfDeleteQueries = (PFNGLDELETEQUERIES_PROC*) SDL_GL_GetProcAddress("glDeleteQueries");
	glpfIsQuery = (PFNGLISQUERY_PROC*) SDL_GL_GetProcAddress("glIsQuery");
	glpfBeginQuery = (PFNGLBEGINQUERY_PROC*) SDL_GL_GetProcAddress("glBeginQuery");
	glpfEndQuery = (PFNGLENDQUERY_PROC*) SDL_GL_GetProcAddress("glEndQuery");
	glpfGetQueryiv = (PFNGLGETQUERYIV_PROC*) SDL_GL_GetProcAddress("glGetQueryiv");
	glpfGetQueryObjectiv = (PFNGLGETQUERYOBJECTIV_PROC*) SDL_GL_GetProcAddress("glGetQueryObjectiv");
	glpfGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIV_PROC*) SDL_GL_GetProcAddress("glGetQueryObjectuiv");
	glpfBindBuffer = (PFNGLBINDBUFFER_PROC*) SDL_GL_GetProcAddress("glBindBuffer");
	glpfDeleteBuffers = (PFNGLDELETEBUFFERS_PROC*) SDL_GL_GetProcAddress("glDeleteBuffers");
	glpfGenBuffers = (PFNGLGENBUFFERS_PROC*) SDL_GL_GetProcAddress("glGenBuffers");
	glpfIsBuffer = (PFNGLISBUFFER_PROC*) SDL_GL_GetProcAddress("glIsBuffer");
	glpfBufferData = (PFNGLBUFFERDATA_PROC*) SDL_GL_GetProcAddress("glBufferData");
	glpfBufferSubData = (PFNGLBUFFERSUBDATA_PROC*) SDL_GL_GetProcAddress("glBufferSubData");
	glpfGetBufferSubData = (PFNGLGETBUFFERSUBDATA_PROC*) SDL_GL_GetProcAddress("glGetBufferSubData");
	glpfMapBuffer = (PFNGLMAPBUFFER_PROC*) SDL_GL_GetProcAddress("glMapBuffer");
	glpfUnmapBuffer = (PFNGLUNMAPBUFFER_PROC*) SDL_GL_GetProcAddress("glUnmapBuffer");
	glpfGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIV_PROC*) SDL_GL_GetProcAddress("glGetBufferParameteriv");
	glpfGetBufferPointerv = (PFNGLGETBUFFERPOINTERV_PROC*) SDL_GL_GetProcAddress("glGetBufferPointerv");

	/* GL_VERSION_2_0 */

	glpfBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATE_PROC*) SDL_GL_GetProcAddress("glBlendEquationSeparate");
	glpfDrawBuffers = (PFNGLDRAWBUFFERS_PROC*) SDL_GL_GetProcAddress("glDrawBuffers");
	glpfStencilOpSeparate = (PFNGLSTENCILOPSEPARATE_PROC*) SDL_GL_GetProcAddress("glStencilOpSeparate");
	glpfStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATE_PROC*) SDL_GL_GetProcAddress("glStencilFuncSeparate");
	glpfStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATE_PROC*) SDL_GL_GetProcAddress("glStencilMaskSeparate");
	glpfAttachShader = (PFNGLATTACHSHADER_PROC*) SDL_GL_GetProcAddress("glAttachShader");
	glpfBindAttribLocation = (PFNGLBINDATTRIBLOCATION_PROC*) SDL_GL_GetProcAddress("glBindAttribLocation");
	glpfCompileShader = (PFNGLCOMPILESHADER_PROC*) SDL_GL_GetProcAddress("glCompileShader");
	glpfCreateProgram = (PFNGLCREATEPROGRAM_PROC*) SDL_GL_GetProcAddress("glCreateProgram");
	glpfCreateShader = (PFNGLCREATESHADER_PROC*) SDL_GL_GetProcAddress("glCreateShader");
	glpfDeleteProgram = (PFNGLDELETEPROGRAM_PROC*) SDL_GL_GetProcAddress("glDeleteProgram");
	glpfDeleteShader = (PFNGLDELETESHADER_PROC*) SDL_GL_GetProcAddress("glDeleteShader");
	glpfDetachShader = (PFNGLDETACHSHADER_PROC*) SDL_GL_GetProcAddress("glDetachShader");
	glpfDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAY_PROC*) SDL_GL_GetProcAddress("glDisableVertexAttribArray");
	glpfEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAY_PROC*) SDL_GL_GetProcAddress("glEnableVertexAttribArray");
	glpfGetActiveAttrib = (PFNGLGETACTIVEATTRIB_PROC*) SDL_GL_GetProcAddress("glGetActiveAttrib");
	glpfGetActiveUniform = (PFNGLGETACTIVEUNIFORM_PROC*) SDL_GL_GetProcAddress("glGetActiveUniform");
	glpfGetAttachedShaders = (PFNGLGETATTACHEDSHADERS_PROC*) SDL_GL_GetProcAddress("glGetAttachedShaders");
	glpfGetAttribLocation = (PFNGLGETATTRIBLOCATION_PROC*) SDL_GL_GetProcAddress("glGetAttribLocation");
	glpfGetProgramiv = (PFNGLGETPROGRAMIV_PROC*) SDL_GL_GetProcAddress("glGetProgramiv");
	glpfGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOG_PROC*) SDL_GL_GetProcAddress("glGetProgramInfoLog");
	glpfGetShaderiv = (PFNGLGETSHADERIV_PROC*) SDL_GL_GetProcAddress("glGetShaderiv");
	glpfGetShaderInfoLog = (PFNGLGETSHADERINFOLOG_PROC*) SDL_GL_GetProcAddress("glGetShaderInfoLog");
	glpfGetShaderSource = (PFNGLGETSHADERSOURCE_PROC*) SDL_GL_GetProcAddress("glGetShaderSource");
	glpfGetUniformLocation = (PFNGLGETUNIFORMLOCATION_PROC*) SDL_GL_GetProcAddress("glGetUniformLocation");
	glpfGetUniformfv = (PFNGLGETUNIFORMFV_PROC*) SDL_GL_GetProcAddress("glGetUniformfv");
	glpfGetUniformiv = (PFNGLGETUNIFORMIV_PROC*) SDL_GL_GetProcAddress("glGetUniformiv");
	glpfGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDV_PROC*) SDL_GL_GetProcAddress("glGetVertexAttribdv");
	glpfGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFV_PROC*) SDL_GL_GetProcAddress("glGetVertexAttribfv");
	glpfGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIV_PROC*) SDL_GL_GetProcAddress("glGetVertexAttribiv");
	glpfGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERV_PROC*) SDL_GL_GetProcAddress("glGetVertexAttribPointerv");
	glpfIsProgram = (PFNGLISPROGRAM_PROC*) SDL_GL_GetProcAddress("glIsProgram");
	glpfIsShader = (PFNGLISSHADER_PROC*) SDL_GL_GetProcAddress("glIsShader");
	glpfLinkProgram = (PFNGLLINKPROGRAM_PROC*) SDL_GL_GetProcAddress("glLinkProgram");
	glpfShaderSource = (PFNGLSHADERSOURCE_PROC*) SDL_GL_GetProcAddress("glShaderSource");
	glpfUseProgram = (PFNGLUSEPROGRAM_PROC*) SDL_GL_GetProcAddress("glUseProgram");
	glpfUniform1f = (PFNGLUNIFORM1F_PROC*) SDL_GL_GetProcAddress("glUniform1f");
	glpfUniform2f = (PFNGLUNIFORM2F_PROC*) SDL_GL_GetProcAddress("glUniform2f");
	glpfUniform3f = (PFNGLUNIFORM3F_PROC*) SDL_GL_GetProcAddress("glUniform3f");
	glpfUniform4f = (PFNGLUNIFORM4F_PROC*) SDL_GL_GetProcAddress("glUniform4f");
	glpfUniform1i = (PFNGLUNIFORM1I_PROC*) SDL_GL_GetProcAddress("glUniform1i");
	glpfUniform2i = (PFNGLUNIFORM2I_PROC*) SDL_GL_GetProcAddress("glUniform2i");
	glpfUniform3i = (PFNGLUNIFORM3I_PROC*) SDL_GL_GetProcAddress("glUniform3i");
	glpfUniform4i = (PFNGLUNIFORM4I_PROC*) SDL_GL_GetProcAddress("glUniform4i");
	glpfUniform1fv = (PFNGLUNIFORM1FV_PROC*) SDL_GL_GetProcAddress("glUniform1fv");
	glpfUniform2fv = (PFNGLUNIFORM2FV_PROC*) SDL_GL_GetProcAddress("glUniform2fv");
	glpfUniform3fv = (PFNGLUNIFORM3FV_PROC*) SDL_GL_GetProcAddress("glUniform3fv");
	glpfUniform4fv = (PFNGLUNIFORM4FV_PROC*) SDL_GL_GetProcAddress("glUniform4fv");
	glpfUniform1iv = (PFNGLUNIFORM1IV_PROC*) SDL_GL_GetProcAddress("glUniform1iv");
	glpfUniform2iv = (PFNGLUNIFORM2IV_PROC*) SDL_GL_GetProcAddress("glUniform2iv");
	glpfUniform3iv = (PFNGLUNIFORM3IV_PROC*) SDL_GL_GetProcAddress("glUniform3iv");
	glpfUniform4iv = (PFNGLUNIFORM4IV_PROC*) SDL_GL_GetProcAddress("glUniform4iv");
	glpfUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FV_PROC*) SDL_GL_GetProcAddress("glUniformMatrix2fv");
	glpfUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FV_PROC*) SDL_GL_GetProcAddress("glUniformMatrix3fv");
	glpfUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FV_PROC*) SDL_GL_GetProcAddress("glUniformMatrix4fv");
	glpfValidateProgram = (PFNGLVALIDATEPROGRAM_PROC*) SDL_GL_GetProcAddress("glValidateProgram");
	glpfVertexAttrib1d = (PFNGLVERTEXATTRIB1D_PROC*) SDL_GL_GetProcAddress("glVertexAttrib1d");
	glpfVertexAttrib1dv = (PFNGLVERTEXATTRIB1DV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib1dv");
	glpfVertexAttrib1f = (PFNGLVERTEXATTRIB1F_PROC*) SDL_GL_GetProcAddress("glVertexAttrib1f");
	glpfVertexAttrib1fv = (PFNGLVERTEXATTRIB1FV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib1fv");
	glpfVertexAttrib1s = (PFNGLVERTEXATTRIB1S_PROC*) SDL_GL_GetProcAddress("glVertexAttrib1s");
	glpfVertexAttrib1sv = (PFNGLVERTEXATTRIB1SV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib1sv");
	glpfVertexAttrib2d = (PFNGLVERTEXATTRIB2D_PROC*) SDL_GL_GetProcAddress("glVertexAttrib2d");
	glpfVertexAttrib2dv = (PFNGLVERTEXATTRIB2DV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib2dv");
	glpfVertexAttrib2f = (PFNGLVERTEXATTRIB2F_PROC*) SDL_GL_GetProcAddress("glVertexAttrib2f");
	glpfVertexAttrib2fv = (PFNGLVERTEXATTRIB2FV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib2fv");
	glpfVertexAttrib2s = (PFNGLVERTEXATTRIB2S_PROC*) SDL_GL_GetProcAddress("glVertexAttrib2s");
	glpfVertexAttrib2sv = (PFNGLVERTEXATTRIB2SV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib2sv");
	glpfVertexAttrib3d = (PFNGLVERTEXATTRIB3D_PROC*) SDL_GL_GetProcAddress("glVertexAttrib3d");
	glpfVertexAttrib3dv = (PFNGLVERTEXATTRIB3DV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib3dv");
	glpfVertexAttrib3f = (PFNGLVERTEXATTRIB3F_PROC*) SDL_GL_GetProcAddress("glVertexAttrib3f");
	glpfVertexAttrib3fv = (PFNGLVERTEXATTRIB3FV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib3fv");
	glpfVertexAttrib3s = (PFNGLVERTEXATTRIB3S_PROC*) SDL_GL_GetProcAddress("glVertexAttrib3s");
	glpfVertexAttrib3sv = (PFNGLVERTEXATTRIB3SV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib3sv");
	glpfVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4Nbv");
	glpfVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4Niv");
	glpfVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4Nsv");
	glpfVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUB_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4Nub");
	glpfVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4Nubv");
	glpfVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4Nuiv");
	glpfVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4Nusv");
	glpfVertexAttrib4bv = (PFNGLVERTEXATTRIB4BV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4bv");
	glpfVertexAttrib4d = (PFNGLVERTEXATTRIB4D_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4d");
	glpfVertexAttrib4dv = (PFNGLVERTEXATTRIB4DV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4dv");
	glpfVertexAttrib4f = (PFNGLVERTEXATTRIB4F_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4f");
	glpfVertexAttrib4fv = (PFNGLVERTEXATTRIB4FV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4fv");
	glpfVertexAttrib4iv = (PFNGLVERTEXATTRIB4IV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4iv");
	glpfVertexAttrib4s = (PFNGLVERTEXATTRIB4S_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4s");
	glpfVertexAttrib4sv = (PFNGLVERTEXATTRIB4SV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4sv");
	glpfVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4ubv");
	glpfVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4uiv");
	glpfVertexAttrib4usv = (PFNGLVERTEXATTRIB4USV_PROC*) SDL_GL_GetProcAddress("glVertexAttrib4usv");
	glpfVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTER_PROC*) SDL_GL_GetProcAddress("glVertexAttribPointer");

	/* GL_VERSION_2_1 */

	glpfUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FV_PROC*) SDL_GL_GetProcAddress("glUniformMatrix2x3fv");
	glpfUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FV_PROC*) SDL_GL_GetProcAddress("glUniformMatrix3x2fv");
	glpfUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FV_PROC*) SDL_GL_GetProcAddress("glUniformMatrix2x4fv");
	glpfUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FV_PROC*) SDL_GL_GetProcAddress("glUniformMatrix4x2fv");
	glpfUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FV_PROC*) SDL_GL_GetProcAddress("glUniformMatrix3x4fv");
	glpfUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FV_PROC*) SDL_GL_GetProcAddress("glUniformMatrix4x3fv");

	/* GL_VERSION_3_0 */

	glpfColorMaski = (PFNGLCOLORMASKI_PROC*) SDL_GL_GetProcAddress("glColorMaski");
	glpfGetBooleani_v = (PFNGLGETBOOLEANI_V_PROC*) SDL_GL_GetProcAddress("glGetBooleani_v");
	glpfGetIntegeri_v = (PFNGLGETINTEGERI_V_PROC*) SDL_GL_GetProcAddress("glGetIntegeri_v");
	glpfEnablei = (PFNGLENABLEI_PROC*) SDL_GL_GetProcAddress("glEnablei");
	glpfDisablei = (PFNGLDISABLEI_PROC*) SDL_GL_GetProcAddress("glDisablei");
	glpfIsEnabledi = (PFNGLISENABLEDI_PROC*) SDL_GL_GetProcAddress("glIsEnabledi");
	glpfBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACK_PROC*) SDL_GL_GetProcAddress("glBeginTransformFeedback");
	glpfEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACK_PROC*) SDL_GL_GetProcAddress("glEndTransformFeedback");
	glpfBindBufferRange = (PFNGLBINDBUFFERRANGE_PROC*) SDL_GL_GetProcAddress("glBindBufferRange");
	glpfBindBufferBase = (PFNGLBINDBUFFERBASE_PROC*) SDL_GL_GetProcAddress("glBindBufferBase");
	glpfTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGS_PROC*) SDL_GL_GetProcAddress("glTransformFeedbackVaryings");
	glpfGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYING_PROC*) SDL_GL_GetProcAddress("glGetTransformFeedbackVarying");
	glpfClampColor = (PFNGLCLAMPCOLOR_PROC*) SDL_GL_GetProcAddress("glClampColor");
	glpfBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDER_PROC*) SDL_GL_GetProcAddress("glBeginConditionalRender");
	glpfEndConditionalRender = (PFNGLENDCONDITIONALRENDER_PROC*) SDL_GL_GetProcAddress("glEndConditionalRender");
	glpfVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTER_PROC*) SDL_GL_GetProcAddress("glVertexAttribIPointer");
	glpfGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIV_PROC*) SDL_GL_GetProcAddress("glGetVertexAttribIiv");
	glpfGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIV_PROC*) SDL_GL_GetProcAddress("glGetVertexAttribIuiv");
	glpfVertexAttribI1i = (PFNGLVERTEXATTRIBI1I_PROC*) SDL_GL_GetProcAddress("glVertexAttribI1i");
	glpfVertexAttribI2i = (PFNGLVERTEXATTRIBI2I_PROC*) SDL_GL_GetProcAddress("glVertexAttribI2i");
	glpfVertexAttribI3i = (PFNGLVERTEXATTRIBI3I_PROC*) SDL_GL_GetProcAddress("glVertexAttribI3i");
	glpfVertexAttribI4i = (PFNGLVERTEXATTRIBI4I_PROC*) SDL_GL_GetProcAddress("glVertexAttribI4i");
	glpfVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UI_PROC*) SDL_GL_GetProcAddress("glVertexAttribI1ui");
	glpfVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UI_PROC*) SDL_GL_GetProcAddress("glVertexAttribI2ui");
	glpfVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UI_PROC*) SDL_GL_GetProcAddress("glVertexAttribI3ui");
	glpfVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UI_PROC*) SDL_GL_GetProcAddress("glVertexAttribI4ui");
	glpfVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI1iv");
	glpfVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI2iv");
	glpfVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI3iv");
	glpfVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI4iv");
	glpfVertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI1uiv");
	glpfVertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI2uiv");
	glpfVertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI3uiv");
	glpfVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI4uiv");
	glpfVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI4bv");
	glpfVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI4sv");
	glpfVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI4ubv");
	glpfVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USV_PROC*) SDL_GL_GetProcAddress("glVertexAttribI4usv");
	glpfGetUniformuiv = (PFNGLGETUNIFORMUIV_PROC*) SDL_GL_GetProcAddress("glGetUniformuiv");
	glpfBindFragDataLocation = (PFNGLBINDFRAGDATALOCATION_PROC*) SDL_GL_GetProcAddress("glBindFragDataLocation");
	glpfGetFragDataLocation = (PFNGLGETFRAGDATALOCATION_PROC*) SDL_GL_GetProcAddress("glGetFragDataLocation");
	glpfUniform1ui = (PFNGLUNIFORM1UI_PROC*) SDL_GL_GetProcAddress("glUniform1ui");
	glpfUniform2ui = (PFNGLUNIFORM2UI_PROC*) SDL_GL_GetProcAddress("glUniform2ui");
	glpfUniform3ui = (PFNGLUNIFORM3UI_PROC*) SDL_GL_GetProcAddress("glUniform3ui");
	glpfUniform4ui = (PFNGLUNIFORM4UI_PROC*) SDL_GL_GetProcAddress("glUniform4ui");
	glpfUniform1uiv = (PFNGLUNIFORM1UIV_PROC*) SDL_GL_GetProcAddress("glUniform1uiv");
	glpfUniform2uiv = (PFNGLUNIFORM2UIV_PROC*) SDL_GL_GetProcAddress("glUniform2uiv");
	glpfUniform3uiv = (PFNGLUNIFORM3UIV_PROC*) SDL_GL_GetProcAddress("glUniform3uiv");
	glpfUniform4uiv = (PFNGLUNIFORM4UIV_PROC*) SDL_GL_GetProcAddress("glUniform4uiv");
	glpfTexParameterIiv = (PFNGLTEXPARAMETERIIV_PROC*) SDL_GL_GetProcAddress("glTexParameterIiv");
	glpfTexParameterIuiv = (PFNGLTEXPARAMETERIUIV_PROC*) SDL_GL_GetProcAddress("glTexParameterIuiv");
	glpfGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIV_PROC*) SDL_GL_GetProcAddress("glGetTexParameterIiv");
	glpfGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIV_PROC*) SDL_GL_GetProcAddress("glGetTexParameterIuiv");
	glpfClearBufferiv = (PFNGLCLEARBUFFERIV_PROC*) SDL_GL_GetProcAddress("glClearBufferiv");
	glpfClearBufferuiv = (PFNGLCLEARBUFFERUIV_PROC*) SDL_GL_GetProcAddress("glClearBufferuiv");
	glpfClearBufferfv = (PFNGLCLEARBUFFERFV_PROC*) SDL_GL_GetProcAddress("glClearBufferfv");
	glpfClearBufferfi = (PFNGLCLEARBUFFERFI_PROC*) SDL_GL_GetProcAddress("glClearBufferfi");
	glpfGetStringi = (PFNGLGETSTRINGI_PROC*) SDL_GL_GetProcAddress("glGetStringi");
	glpfIsRenderbuffer = (PFNGLISRENDERBUFFER_PROC*) SDL_GL_GetProcAddress("glIsRenderbuffer");
	glpfBindRenderbuffer = (PFNGLBINDRENDERBUFFER_PROC*) SDL_GL_GetProcAddress("glBindRenderbuffer");
	glpfDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERS_PROC*) SDL_GL_GetProcAddress("glDeleteRenderbuffers");
	glpfGenRenderbuffers = (PFNGLGENRENDERBUFFERS_PROC*) SDL_GL_GetProcAddress("glGenRenderbuffers");
	glpfRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGE_PROC*) SDL_GL_GetProcAddress("glRenderbufferStorage");
	glpfGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIV_PROC*) SDL_GL_GetProcAddress("glGetRenderbufferParameteriv");
	glpfIsFramebuffer = (PFNGLISFRAMEBUFFER_PROC*) SDL_GL_GetProcAddress("glIsFramebuffer");
	glpfBindFramebuffer = (PFNGLBINDFRAMEBUFFER_PROC*) SDL_GL_GetProcAddress("glBindFramebuffer");
	glpfDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERS_PROC*) SDL_GL_GetProcAddress("glDeleteFramebuffers");
	glpfGenFramebuffers = (PFNGLGENFRAMEBUFFERS_PROC*) SDL_GL_GetProcAddress("glGenFramebuffers");
	glpfCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUS_PROC*) SDL_GL_GetProcAddress("glCheckFramebufferStatus");
	glpfFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1D_PROC*) SDL_GL_GetProcAddress("glFramebufferTexture1D");
	glpfFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2D_PROC*) SDL_GL_GetProcAddress("glFramebufferTexture2D");
	glpfFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3D_PROC*) SDL_GL_GetProcAddress("glFramebufferTexture3D");
	glpfFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFER_PROC*) SDL_GL_GetProcAddress("glFramebufferRenderbuffer");
	glpfGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIV_PROC*) SDL_GL_GetProcAddress("glGetFramebufferAttachmentParameteriv");
	glpfGenerateMipmap = (PFNGLGENERATEMIPMAP_PROC*) SDL_GL_GetProcAddress("glGenerateMipmap");
	glpfBlitFramebuffer = (PFNGLBLITFRAMEBUFFER_PROC*) SDL_GL_GetProcAddress("glBlitFramebuffer");
	glpfRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLE_PROC*) SDL_GL_GetProcAddress("glRenderbufferStorageMultisample");
	glpfFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYER_PROC*) SDL_GL_GetProcAddress("glFramebufferTextureLayer");
	glpfMapBufferRange = (PFNGLMAPBUFFERRANGE_PROC*) SDL_GL_GetProcAddress("glMapBufferRange");
	glpfFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGE_PROC*) SDL_GL_GetProcAddress("glFlushMappedBufferRange");
	glpfBindVertexArray = (PFNGLBINDVERTEXARRAY_PROC*) SDL_GL_GetProcAddress("glBindVertexArray");
	glpfDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYS_PROC*) SDL_GL_GetProcAddress("glDeleteVertexArrays");
	glpfGenVertexArrays = (PFNGLGENVERTEXARRAYS_PROC*) SDL_GL_GetProcAddress("glGenVertexArrays");
	glpfIsVertexArray = (PFNGLISVERTEXARRAY_PROC*) SDL_GL_GetProcAddress("glIsVertexArray");

	/* GL_VERSION_3_1 */

	glpfDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCED_PROC*) SDL_GL_GetProcAddress("glDrawArraysInstanced");
	glpfDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCED_PROC*) SDL_GL_GetProcAddress("glDrawElementsInstanced");
	glpfTexBuffer = (PFNGLTEXBUFFER_PROC*) SDL_GL_GetProcAddress("glTexBuffer");
	glpfPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEX_PROC*) SDL_GL_GetProcAddress("glPrimitiveRestartIndex");
	glpfCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATA_PROC*) SDL_GL_GetProcAddress("glCopyBufferSubData");
	glpfGetUniformIndices = (PFNGLGETUNIFORMINDICES_PROC*) SDL_GL_GetProcAddress("glGetUniformIndices");
	glpfGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIV_PROC*) SDL_GL_GetProcAddress("glGetActiveUniformsiv");
	glpfGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAME_PROC*) SDL_GL_GetProcAddress("glGetActiveUniformName");
	glpfGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEX_PROC*) SDL_GL_GetProcAddress("glGetUniformBlockIndex");
	glpfGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIV_PROC*) SDL_GL_GetProcAddress("glGetActiveUniformBlockiv");
	glpfGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAME_PROC*) SDL_GL_GetProcAddress("glGetActiveUniformBlockName");
	glpfUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDING_PROC*) SDL_GL_GetProcAddress("glUniformBlockBinding");

	/* GL_VERSION_3_2 */

	glpfDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEX_PROC*) SDL_GL_GetProcAddress("glDrawElementsBaseVertex");
	glpfDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEX_PROC*) SDL_GL_GetProcAddress("glDrawRangeElementsBaseVertex");
	glpfDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEX_PROC*) SDL_GL_GetProcAddress("glDrawElementsInstancedBaseVertex");
	glpfMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEX_PROC*) SDL_GL_GetProcAddress("glMultiDrawElementsBaseVertex");
	glpfProvokingVertex = (PFNGLPROVOKINGVERTEX_PROC*) SDL_GL_GetProcAddress("glProvokingVertex");
	glpfFenceSync = (PFNGLFENCESYNC_PROC*) SDL_GL_GetProcAddress("glFenceSync");
	glpfIsSync = (PFNGLISSYNC_PROC*) SDL_GL_GetProcAddress("glIsSync");
	glpfDeleteSync = (PFNGLDELETESYNC_PROC*) SDL_GL_GetProcAddress("glDeleteSync");
	glpfClientWaitSync = (PFNGLCLIENTWAITSYNC_PROC*) SDL_GL_GetProcAddress("glClientWaitSync");
	glpfWaitSync = (PFNGLWAITSYNC_PROC*) SDL_GL_GetProcAddress("glWaitSync");
	glpfGetInteger64v = (PFNGLGETINTEGER64V_PROC*) SDL_GL_GetProcAddress("glGetInteger64v");
	glpfGetSynciv = (PFNGLGETSYNCIV_PROC*) SDL_GL_GetProcAddress("glGetSynciv");
	glpfGetInteger64i_v = (PFNGLGETINTEGER64I_V_PROC*) SDL_GL_GetProcAddress("glGetInteger64i_v");
	glpfGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64V_PROC*) SDL_GL_GetProcAddress("glGetBufferParameteri64v");
	glpfFramebufferTexture = (PFNGLFRAMEBUFFERTEXTURE_PROC*) SDL_GL_GetProcAddress("glFramebufferTexture");
	glpfTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLE_PROC*) SDL_GL_GetProcAddress("glTexImage2DMultisample");
	glpfTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLE_PROC*) SDL_GL_GetProcAddress("glTexImage3DMultisample");
	glpfGetMultisamplefv = (PFNGLGETMULTISAMPLEFV_PROC*) SDL_GL_GetProcAddress("glGetMultisamplefv");
	glpfSampleMaski = (PFNGLSAMPLEMASKI_PROC*) SDL_GL_GetProcAddress("glSampleMaski");

	/* GL_VERSION_3_3 */

	glpfBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXED_PROC*) SDL_GL_GetProcAddress("glBindFragDataLocationIndexed");
	glpfGetFragDataIndex = (PFNGLGETFRAGDATAINDEX_PROC*) SDL_GL_GetProcAddress("glGetFragDataIndex");
	glpfGenSamplers = (PFNGLGENSAMPLERS_PROC*) SDL_GL_GetProcAddress("glGenSamplers");
	glpfDeleteSamplers = (PFNGLDELETESAMPLERS_PROC*) SDL_GL_GetProcAddress("glDeleteSamplers");
	glpfIsSampler = (PFNGLISSAMPLER_PROC*) SDL_GL_GetProcAddress("glIsSampler");
	glpfBindSampler = (PFNGLBINDSAMPLER_PROC*) SDL_GL_GetProcAddress("glBindSampler");
	glpfSamplerParameteri = (PFNGLSAMPLERPARAMETERI_PROC*) SDL_GL_GetProcAddress("glSamplerParameteri");
	glpfSamplerParameteriv = (PFNGLSAMPLERPARAMETERIV_PROC*) SDL_GL_GetProcAddress("glSamplerParameteriv");
	glpfSamplerParameterf = (PFNGLSAMPLERPARAMETERF_PROC*) SDL_GL_GetProcAddress("glSamplerParameterf");
	glpfSamplerParameterfv = (PFNGLSAMPLERPARAMETERFV_PROC*) SDL_GL_GetProcAddress("glSamplerParameterfv");
	glpfSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIV_PROC*) SDL_GL_GetProcAddress("glSamplerParameterIiv");
	glpfSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIV_PROC*) SDL_GL_GetProcAddress("glSamplerParameterIuiv");
	glpfGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIV_PROC*) SDL_GL_GetProcAddress("glGetSamplerParameteriv");
	glpfGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIV_PROC*) SDL_GL_GetProcAddress("glGetSamplerParameterIiv");
	glpfGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFV_PROC*) SDL_GL_GetProcAddress("glGetSamplerParameterfv");
	glpfGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIV_PROC*) SDL_GL_GetProcAddress("glGetSamplerParameterIuiv");
	glpfQueryCounter = (PFNGLQUERYCOUNTER_PROC*) SDL_GL_GetProcAddress("glQueryCounter");
	glpfGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64V_PROC*) SDL_GL_GetProcAddress("glGetQueryObjecti64v");
	glpfGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64V_PROC*) SDL_GL_GetProcAddress("glGetQueryObjectui64v");
	glpfVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISOR_PROC*) SDL_GL_GetProcAddress("glVertexAttribDivisor");
	glpfVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UI_PROC*) SDL_GL_GetProcAddress("glVertexAttribP1ui");
	glpfVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIV_PROC*) SDL_GL_GetProcAddress("glVertexAttribP1uiv");
	glpfVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UI_PROC*) SDL_GL_GetProcAddress("glVertexAttribP2ui");
	glpfVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIV_PROC*) SDL_GL_GetProcAddress("glVertexAttribP2uiv");
	glpfVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UI_PROC*) SDL_GL_GetProcAddress("glVertexAttribP3ui");
	glpfVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIV_PROC*) SDL_GL_GetProcAddress("glVertexAttribP3uiv");
	glpfVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UI_PROC*) SDL_GL_GetProcAddress("glVertexAttribP4ui");
	glpfVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIV_PROC*) SDL_GL_GetProcAddress("glVertexAttribP4uiv");

	/* GL_ARB_debug_output */

	glpfDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARB_PROC*) SDL_GL_GetProcAddress("glDebugMessageControlARB");
	glpfDebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARB_PROC*) SDL_GL_GetProcAddress("glDebugMessageInsertARB");
	glpfDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARB_PROC*) SDL_GL_GetProcAddress("glDebugMessageCallbackARB");
	glpfGetDebugMessageLogARB = (PFNGLGETDEBUGMESSAGELOGARB_PROC*) SDL_GL_GetProcAddress("glGetDebugMessageLogARB");

	/* GL_ARB_direct_state_access */

	glpfCreateBuffersARB = (PFNGLCREATEBUFFERSARB_PROC*) SDL_GL_GetProcAddress("glCreateBuffersARB");
	glpfNamedBufferStorageARB = (PFNGLNAMEDBUFFERSTORAGEARB_PROC*) SDL_GL_GetProcAddress("glNamedBufferStorageARB");
	glpfNamedBufferDataARB = (PFNGLNAMEDBUFFERDATAARB_PROC*) SDL_GL_GetProcAddress("glNamedBufferDataARB");
	glpfNamedBufferSubDataARB = (PFNGLNAMEDBUFFERSUBDATAARB_PROC*) SDL_GL_GetProcAddress("glNamedBufferSubDataARB");
	glpfCopyNamedBufferSubDataARB = (PFNGLCOPYNAMEDBUFFERSUBDATAARB_PROC*) SDL_GL_GetProcAddress("glCreateBuffersARB");
	glpfClearNamedBufferDataARB = (PFNGLCLEARNAMEDBUFFERDATAARB_PROC*) SDL_GL_GetProcAddress("glCopyNamedBufferSubDataARB");
	glpfClearNamedBufferSubDataARB = (PFNGLCLEARNAMEDBUFFERSUBDATAARB_PROC*) SDL_GL_GetProcAddress("glClearNamedBufferSubDataARB");
	glpfMapNamedBufferARB = (PFNGLMAPNAMEDBUFFERARB_PROC*) SDL_GL_GetProcAddress("glMapNamedBufferARB");
	glpfMapNamedBufferRangeARB = (PFNGLMAPNAMEDBUFFERRANGEARB_PROC*) SDL_GL_GetProcAddress("glMapNamedBufferRangeARB");
	glpfUnmapNamedBufferARB = (PFNGLUNMAPNAMEDBUFFERARB_PROC*) SDL_GL_GetProcAddress("glUnmapNamedBufferARB");
	glpfFlushMappedNamedBufferRangeARB = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEARB_PROC*) SDL_GL_GetProcAddress("glFlushMappedNamedBufferRangeARB");
	glpfGetNamedBufferParameterivARB = (PFNGLGETNAMEDBUFFERPARAMETERIVARB_PROC*) SDL_GL_GetProcAddress("glGetNamedBufferParameterivARB");
	glpfGetNamedBufferParameteri64vARB = (PFNGLGETNAMEDBUFFERPARAMETERI64VARB_PROC*) SDL_GL_GetProcAddress("glGetNamedBufferParameteri64vARB");
	glpfGetNamedBufferPointervARB = (PFNGLGETNAMEDBUFFERPOINTERVARB_PROC*) SDL_GL_GetProcAddress("glGetNamedBufferPointervARB");
	glpfGetNamedBufferSubDataARB = (PFNGLGETNAMEDBUFFERSUBDATAARB_PROC*) SDL_GL_GetProcAddress("glGetNamedBufferSubDataARB");
}

/* GL_VERSION_1_2 */

PFNGLDRAWRANGEELEMENTS_PROC* glpfDrawRangeElements = NULL;
PFNGLTEXIMAGE3D_PROC* glpfTexImage3D = NULL;
PFNGLTEXSUBIMAGE3D_PROC* glpfTexSubImage3D = NULL;
PFNGLCOPYTEXSUBIMAGE3D_PROC* glpfCopyTexSubImage3D = NULL;

/* GL_VERSION_1_3 */

PFNGLACTIVETEXTURE_PROC* glpfActiveTexture = NULL;
PFNGLSAMPLECOVERAGE_PROC* glpfSampleCoverage = NULL;
PFNGLCOMPRESSEDTEXIMAGE3D_PROC* glpfCompressedTexImage3D = NULL;
PFNGLCOMPRESSEDTEXIMAGE2D_PROC* glpfCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXIMAGE1D_PROC* glpfCompressedTexImage1D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3D_PROC* glpfCompressedTexSubImage3D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2D_PROC* glpfCompressedTexSubImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1D_PROC* glpfCompressedTexSubImage1D = NULL;
PFNGLGETCOMPRESSEDTEXIMAGE_PROC* glpfGetCompressedTexImage = NULL;

/* GL_VERSION_1_4 */

PFNGLBLENDFUNCSEPARATE_PROC* glpfBlendFuncSeparate = NULL;
PFNGLMULTIDRAWARRAYS_PROC* glpfMultiDrawArrays = NULL;
PFNGLMULTIDRAWELEMENTS_PROC* glpfMultiDrawElements = NULL;
PFNGLPOINTPARAMETERF_PROC* glpfPointParameterf = NULL;
PFNGLPOINTPARAMETERFV_PROC* glpfPointParameterfv = NULL;
PFNGLPOINTPARAMETERI_PROC* glpfPointParameteri = NULL;
PFNGLPOINTPARAMETERIV_PROC* glpfPointParameteriv = NULL;
PFNGLBLENDCOLOR_PROC* glpfBlendColor = NULL;
PFNGLBLENDEQUATION_PROC* glpfBlendEquation = NULL;

/* GL_VERSION_1_5 */

PFNGLGENQUERIES_PROC* glpfGenQueries = NULL;
PFNGLDELETEQUERIES_PROC* glpfDeleteQueries = NULL;
PFNGLISQUERY_PROC* glpfIsQuery = NULL;
PFNGLBEGINQUERY_PROC* glpfBeginQuery = NULL;
PFNGLENDQUERY_PROC* glpfEndQuery = NULL;
PFNGLGETQUERYIV_PROC* glpfGetQueryiv = NULL;
PFNGLGETQUERYOBJECTIV_PROC* glpfGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUIV_PROC* glpfGetQueryObjectuiv = NULL;
PFNGLBINDBUFFER_PROC* glpfBindBuffer = NULL;
PFNGLDELETEBUFFERS_PROC* glpfDeleteBuffers = NULL;
PFNGLGENBUFFERS_PROC* glpfGenBuffers = NULL;
PFNGLISBUFFER_PROC* glpfIsBuffer = NULL;
PFNGLBUFFERDATA_PROC* glpfBufferData = NULL;
PFNGLBUFFERSUBDATA_PROC* glpfBufferSubData = NULL;
PFNGLGETBUFFERSUBDATA_PROC* glpfGetBufferSubData = NULL;
PFNGLMAPBUFFER_PROC* glpfMapBuffer = NULL;
PFNGLUNMAPBUFFER_PROC* glpfUnmapBuffer = NULL;
PFNGLGETBUFFERPARAMETERIV_PROC* glpfGetBufferParameteriv = NULL;
PFNGLGETBUFFERPOINTERV_PROC* glpfGetBufferPointerv = NULL;

/* GL_VERSION_2_0 */

PFNGLBLENDEQUATIONSEPARATE_PROC* glpfBlendEquationSeparate = NULL;
PFNGLDRAWBUFFERS_PROC* glpfDrawBuffers = NULL;
PFNGLSTENCILOPSEPARATE_PROC* glpfStencilOpSeparate = NULL;
PFNGLSTENCILFUNCSEPARATE_PROC* glpfStencilFuncSeparate = NULL;
PFNGLSTENCILMASKSEPARATE_PROC* glpfStencilMaskSeparate = NULL;
PFNGLATTACHSHADER_PROC* glpfAttachShader = NULL;
PFNGLBINDATTRIBLOCATION_PROC* glpfBindAttribLocation = NULL;
PFNGLCOMPILESHADER_PROC* glpfCompileShader = NULL;
PFNGLCREATEPROGRAM_PROC* glpfCreateProgram = NULL;
PFNGLCREATESHADER_PROC* glpfCreateShader = NULL;
PFNGLDELETEPROGRAM_PROC* glpfDeleteProgram = NULL;
PFNGLDELETESHADER_PROC* glpfDeleteShader = NULL;
PFNGLDETACHSHADER_PROC* glpfDetachShader = NULL;
PFNGLDISABLEVERTEXATTRIBARRAY_PROC* glpfDisableVertexAttribArray = NULL;
PFNGLENABLEVERTEXATTRIBARRAY_PROC* glpfEnableVertexAttribArray = NULL;
PFNGLGETACTIVEATTRIB_PROC* glpfGetActiveAttrib = NULL;
PFNGLGETACTIVEUNIFORM_PROC* glpfGetActiveUniform = NULL;
PFNGLGETATTACHEDSHADERS_PROC* glpfGetAttachedShaders = NULL;
PFNGLGETATTRIBLOCATION_PROC* glpfGetAttribLocation = NULL;
PFNGLGETPROGRAMIV_PROC* glpfGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOG_PROC* glpfGetProgramInfoLog = NULL;
PFNGLGETSHADERIV_PROC* glpfGetShaderiv = NULL;
PFNGLGETSHADERINFOLOG_PROC* glpfGetShaderInfoLog = NULL;
PFNGLGETSHADERSOURCE_PROC* glpfGetShaderSource = NULL;
PFNGLGETUNIFORMLOCATION_PROC* glpfGetUniformLocation = NULL;
PFNGLGETUNIFORMFV_PROC* glpfGetUniformfv = NULL;
PFNGLGETUNIFORMIV_PROC* glpfGetUniformiv = NULL;
PFNGLGETVERTEXATTRIBDV_PROC* glpfGetVertexAttribdv = NULL;
PFNGLGETVERTEXATTRIBFV_PROC* glpfGetVertexAttribfv = NULL;
PFNGLGETVERTEXATTRIBIV_PROC* glpfGetVertexAttribiv = NULL;
PFNGLGETVERTEXATTRIBPOINTERV_PROC* glpfGetVertexAttribPointerv = NULL;
PFNGLISPROGRAM_PROC* glpfIsProgram = NULL;
PFNGLISSHADER_PROC* glpfIsShader = NULL;
PFNGLLINKPROGRAM_PROC* glpfLinkProgram = NULL;
PFNGLSHADERSOURCE_PROC* glpfShaderSource = NULL;
PFNGLUSEPROGRAM_PROC* glpfUseProgram = NULL;
PFNGLUNIFORM1F_PROC* glpfUniform1f = NULL;
PFNGLUNIFORM2F_PROC* glpfUniform2f = NULL;
PFNGLUNIFORM3F_PROC* glpfUniform3f = NULL;
PFNGLUNIFORM4F_PROC* glpfUniform4f = NULL;
PFNGLUNIFORM1I_PROC* glpfUniform1i = NULL;
PFNGLUNIFORM2I_PROC* glpfUniform2i = NULL;
PFNGLUNIFORM3I_PROC* glpfUniform3i = NULL;
PFNGLUNIFORM4I_PROC* glpfUniform4i = NULL;
PFNGLUNIFORM1FV_PROC* glpfUniform1fv = NULL;
PFNGLUNIFORM2FV_PROC* glpfUniform2fv = NULL;
PFNGLUNIFORM3FV_PROC* glpfUniform3fv = NULL;
PFNGLUNIFORM4FV_PROC* glpfUniform4fv = NULL;
PFNGLUNIFORM1IV_PROC* glpfUniform1iv = NULL;
PFNGLUNIFORM2IV_PROC* glpfUniform2iv = NULL;
PFNGLUNIFORM3IV_PROC* glpfUniform3iv = NULL;
PFNGLUNIFORM4IV_PROC* glpfUniform4iv = NULL;
PFNGLUNIFORMMATRIX2FV_PROC* glpfUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX3FV_PROC* glpfUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX4FV_PROC* glpfUniformMatrix4fv = NULL;
PFNGLVALIDATEPROGRAM_PROC* glpfValidateProgram = NULL;
PFNGLVERTEXATTRIB1D_PROC* glpfVertexAttrib1d = NULL;
PFNGLVERTEXATTRIB1DV_PROC* glpfVertexAttrib1dv = NULL;
PFNGLVERTEXATTRIB1F_PROC* glpfVertexAttrib1f = NULL;
PFNGLVERTEXATTRIB1FV_PROC* glpfVertexAttrib1fv = NULL;
PFNGLVERTEXATTRIB1S_PROC* glpfVertexAttrib1s = NULL;
PFNGLVERTEXATTRIB1SV_PROC* glpfVertexAttrib1sv = NULL;
PFNGLVERTEXATTRIB2D_PROC* glpfVertexAttrib2d = NULL;
PFNGLVERTEXATTRIB2DV_PROC* glpfVertexAttrib2dv = NULL;
PFNGLVERTEXATTRIB2F_PROC* glpfVertexAttrib2f = NULL;
PFNGLVERTEXATTRIB2FV_PROC* glpfVertexAttrib2fv = NULL;
PFNGLVERTEXATTRIB2S_PROC* glpfVertexAttrib2s = NULL;
PFNGLVERTEXATTRIB2SV_PROC* glpfVertexAttrib2sv = NULL;
PFNGLVERTEXATTRIB3D_PROC* glpfVertexAttrib3d = NULL;
PFNGLVERTEXATTRIB3DV_PROC* glpfVertexAttrib3dv = NULL;
PFNGLVERTEXATTRIB3F_PROC* glpfVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FV_PROC* glpfVertexAttrib3fv = NULL;
PFNGLVERTEXATTRIB3S_PROC* glpfVertexAttrib3s = NULL;
PFNGLVERTEXATTRIB3SV_PROC* glpfVertexAttrib3sv = NULL;
PFNGLVERTEXATTRIB4NBV_PROC* glpfVertexAttrib4Nbv = NULL;
PFNGLVERTEXATTRIB4NIV_PROC* glpfVertexAttrib4Niv = NULL;
PFNGLVERTEXATTRIB4NSV_PROC* glpfVertexAttrib4Nsv = NULL;
PFNGLVERTEXATTRIB4NUB_PROC* glpfVertexAttrib4Nub = NULL;
PFNGLVERTEXATTRIB4NUBV_PROC* glpfVertexAttrib4Nubv = NULL;
PFNGLVERTEXATTRIB4NUIV_PROC* glpfVertexAttrib4Nuiv = NULL;
PFNGLVERTEXATTRIB4NUSV_PROC* glpfVertexAttrib4Nusv = NULL;
PFNGLVERTEXATTRIB4BV_PROC* glpfVertexAttrib4bv = NULL;
PFNGLVERTEXATTRIB4D_PROC* glpfVertexAttrib4d = NULL;
PFNGLVERTEXATTRIB4DV_PROC* glpfVertexAttrib4dv = NULL;
PFNGLVERTEXATTRIB4F_PROC* glpfVertexAttrib4f = NULL;
PFNGLVERTEXATTRIB4FV_PROC* glpfVertexAttrib4fv = NULL;
PFNGLVERTEXATTRIB4IV_PROC* glpfVertexAttrib4iv = NULL;
PFNGLVERTEXATTRIB4S_PROC* glpfVertexAttrib4s = NULL;
PFNGLVERTEXATTRIB4SV_PROC* glpfVertexAttrib4sv = NULL;
PFNGLVERTEXATTRIB4UBV_PROC* glpfVertexAttrib4ubv = NULL;
PFNGLVERTEXATTRIB4UIV_PROC* glpfVertexAttrib4uiv = NULL;
PFNGLVERTEXATTRIB4USV_PROC* glpfVertexAttrib4usv = NULL;
PFNGLVERTEXATTRIBPOINTER_PROC* glpfVertexAttribPointer = NULL;

/* GL_VERSION_2_1 */

PFNGLUNIFORMMATRIX2X3FV_PROC* glpfUniformMatrix2x3fv = NULL;
PFNGLUNIFORMMATRIX3X2FV_PROC* glpfUniformMatrix3x2fv = NULL;
PFNGLUNIFORMMATRIX2X4FV_PROC* glpfUniformMatrix2x4fv = NULL;
PFNGLUNIFORMMATRIX4X2FV_PROC* glpfUniformMatrix4x2fv = NULL;
PFNGLUNIFORMMATRIX3X4FV_PROC* glpfUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4X3FV_PROC* glpfUniformMatrix4x3fv = NULL;

/* GL_VERSION_3_0 */

PFNGLCOLORMASKI_PROC* glpfColorMaski = NULL;
PFNGLGETBOOLEANI_V_PROC* glpfGetBooleani_v = NULL;
PFNGLGETINTEGERI_V_PROC* glpfGetIntegeri_v = NULL;
PFNGLENABLEI_PROC* glpfEnablei = NULL;
PFNGLDISABLEI_PROC* glpfDisablei = NULL;
PFNGLISENABLEDI_PROC* glpfIsEnabledi = NULL;
PFNGLBEGINTRANSFORMFEEDBACK_PROC* glpfBeginTransformFeedback = NULL;
PFNGLENDTRANSFORMFEEDBACK_PROC* glpfEndTransformFeedback = NULL;
PFNGLBINDBUFFERRANGE_PROC* glpfBindBufferRange = NULL;
PFNGLBINDBUFFERBASE_PROC* glpfBindBufferBase = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGS_PROC* glpfTransformFeedbackVaryings = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYING_PROC* glpfGetTransformFeedbackVarying = NULL;
PFNGLCLAMPCOLOR_PROC* glpfClampColor = NULL;
PFNGLBEGINCONDITIONALRENDER_PROC* glpfBeginConditionalRender = NULL;
PFNGLENDCONDITIONALRENDER_PROC* glpfEndConditionalRender = NULL;
PFNGLVERTEXATTRIBIPOINTER_PROC* glpfVertexAttribIPointer = NULL;
PFNGLGETVERTEXATTRIBIIV_PROC* glpfGetVertexAttribIiv = NULL;
PFNGLGETVERTEXATTRIBIUIV_PROC* glpfGetVertexAttribIuiv = NULL;
PFNGLVERTEXATTRIBI1I_PROC* glpfVertexAttribI1i = NULL;
PFNGLVERTEXATTRIBI2I_PROC* glpfVertexAttribI2i = NULL;
PFNGLVERTEXATTRIBI3I_PROC* glpfVertexAttribI3i = NULL;
PFNGLVERTEXATTRIBI4I_PROC* glpfVertexAttribI4i = NULL;
PFNGLVERTEXATTRIBI1UI_PROC* glpfVertexAttribI1ui = NULL;
PFNGLVERTEXATTRIBI2UI_PROC* glpfVertexAttribI2ui = NULL;
PFNGLVERTEXATTRIBI3UI_PROC* glpfVertexAttribI3ui = NULL;
PFNGLVERTEXATTRIBI4UI_PROC* glpfVertexAttribI4ui = NULL;
PFNGLVERTEXATTRIBI1IV_PROC* glpfVertexAttribI1iv = NULL;
PFNGLVERTEXATTRIBI2IV_PROC* glpfVertexAttribI2iv = NULL;
PFNGLVERTEXATTRIBI3IV_PROC* glpfVertexAttribI3iv = NULL;
PFNGLVERTEXATTRIBI4IV_PROC* glpfVertexAttribI4iv = NULL;
PFNGLVERTEXATTRIBI1UIV_PROC* glpfVertexAttribI1uiv = NULL;
PFNGLVERTEXATTRIBI2UIV_PROC* glpfVertexAttribI2uiv = NULL;
PFNGLVERTEXATTRIBI3UIV_PROC* glpfVertexAttribI3uiv = NULL;
PFNGLVERTEXATTRIBI4UIV_PROC* glpfVertexAttribI4uiv = NULL;
PFNGLVERTEXATTRIBI4BV_PROC* glpfVertexAttribI4bv = NULL;
PFNGLVERTEXATTRIBI4SV_PROC* glpfVertexAttribI4sv = NULL;
PFNGLVERTEXATTRIBI4UBV_PROC* glpfVertexAttribI4ubv = NULL;
PFNGLVERTEXATTRIBI4USV_PROC* glpfVertexAttribI4usv = NULL;
PFNGLGETUNIFORMUIV_PROC* glpfGetUniformuiv = NULL;
PFNGLBINDFRAGDATALOCATION_PROC* glpfBindFragDataLocation = NULL;
PFNGLGETFRAGDATALOCATION_PROC* glpfGetFragDataLocation = NULL;
PFNGLUNIFORM1UI_PROC* glpfUniform1ui = NULL;
PFNGLUNIFORM2UI_PROC* glpfUniform2ui = NULL;
PFNGLUNIFORM3UI_PROC* glpfUniform3ui = NULL;
PFNGLUNIFORM4UI_PROC* glpfUniform4ui = NULL;
PFNGLUNIFORM1UIV_PROC* glpfUniform1uiv = NULL;
PFNGLUNIFORM2UIV_PROC* glpfUniform2uiv = NULL;
PFNGLUNIFORM3UIV_PROC* glpfUniform3uiv = NULL;
PFNGLUNIFORM4UIV_PROC* glpfUniform4uiv = NULL;
PFNGLTEXPARAMETERIIV_PROC* glpfTexParameterIiv = NULL;
PFNGLTEXPARAMETERIUIV_PROC* glpfTexParameterIuiv = NULL;
PFNGLGETTEXPARAMETERIIV_PROC* glpfGetTexParameterIiv = NULL;
PFNGLGETTEXPARAMETERIUIV_PROC* glpfGetTexParameterIuiv = NULL;
PFNGLCLEARBUFFERIV_PROC* glpfClearBufferiv = NULL;
PFNGLCLEARBUFFERUIV_PROC* glpfClearBufferuiv = NULL;
PFNGLCLEARBUFFERFV_PROC* glpfClearBufferfv = NULL;
PFNGLCLEARBUFFERFI_PROC* glpfClearBufferfi = NULL;
PFNGLGETSTRINGI_PROC* glpfGetStringi = NULL;
PFNGLISRENDERBUFFER_PROC* glpfIsRenderbuffer = NULL;
PFNGLBINDRENDERBUFFER_PROC* glpfBindRenderbuffer = NULL;
PFNGLDELETERENDERBUFFERS_PROC* glpfDeleteRenderbuffers = NULL;
PFNGLGENRENDERBUFFERS_PROC* glpfGenRenderbuffers = NULL;
PFNGLRENDERBUFFERSTORAGE_PROC* glpfRenderbufferStorage = NULL;
PFNGLGETRENDERBUFFERPARAMETERIV_PROC* glpfGetRenderbufferParameteriv = NULL;
PFNGLISFRAMEBUFFER_PROC* glpfIsFramebuffer = NULL;
PFNGLBINDFRAMEBUFFER_PROC* glpfBindFramebuffer = NULL;
PFNGLDELETEFRAMEBUFFERS_PROC* glpfDeleteFramebuffers = NULL;
PFNGLGENFRAMEBUFFERS_PROC* glpfGenFramebuffers = NULL;
PFNGLCHECKFRAMEBUFFERSTATUS_PROC* glpfCheckFramebufferStatus = NULL;
PFNGLFRAMEBUFFERTEXTURE1D_PROC* glpfFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2D_PROC* glpfFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3D_PROC* glpfFramebufferTexture3D = NULL;
PFNGLFRAMEBUFFERRENDERBUFFER_PROC* glpfFramebufferRenderbuffer = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIV_PROC* glpfGetFramebufferAttachmentParameteriv = NULL;
PFNGLGENERATEMIPMAP_PROC* glpfGenerateMipmap = NULL;
PFNGLBLITFRAMEBUFFER_PROC* glpfBlitFramebuffer = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLE_PROC* glpfRenderbufferStorageMultisample = NULL;
PFNGLFRAMEBUFFERTEXTURELAYER_PROC* glpfFramebufferTextureLayer = NULL;
PFNGLMAPBUFFERRANGE_PROC* glpfMapBufferRange = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGE_PROC* glpfFlushMappedBufferRange = NULL;
PFNGLBINDVERTEXARRAY_PROC* glpfBindVertexArray = NULL;
PFNGLDELETEVERTEXARRAYS_PROC* glpfDeleteVertexArrays = NULL;
PFNGLGENVERTEXARRAYS_PROC* glpfGenVertexArrays = NULL;
PFNGLISVERTEXARRAY_PROC* glpfIsVertexArray = NULL;

/* GL_VERSION_3_1 */

PFNGLDRAWARRAYSINSTANCED_PROC* glpfDrawArraysInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCED_PROC* glpfDrawElementsInstanced = NULL;
PFNGLTEXBUFFER_PROC* glpfTexBuffer = NULL;
PFNGLPRIMITIVERESTARTINDEX_PROC* glpfPrimitiveRestartIndex = NULL;
PFNGLCOPYBUFFERSUBDATA_PROC* glpfCopyBufferSubData = NULL;
PFNGLGETUNIFORMINDICES_PROC* glpfGetUniformIndices = NULL;
PFNGLGETACTIVEUNIFORMSIV_PROC* glpfGetActiveUniformsiv = NULL;
PFNGLGETACTIVEUNIFORMNAME_PROC* glpfGetActiveUniformName = NULL;
PFNGLGETUNIFORMBLOCKINDEX_PROC* glpfGetUniformBlockIndex = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIV_PROC* glpfGetActiveUniformBlockiv = NULL;
PFNGLGETACTIVEUNIFORMBLOCKNAME_PROC* glpfGetActiveUniformBlockName = NULL;
PFNGLUNIFORMBLOCKBINDING_PROC* glpfUniformBlockBinding = NULL;

/* GL_VERSION_3_2 */

PFNGLDRAWELEMENTSBASEVERTEX_PROC* glpfDrawElementsBaseVertex = NULL;
PFNGLDRAWRANGEELEMENTSBASEVERTEX_PROC* glpfDrawRangeElementsBaseVertex = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEX_PROC* glpfDrawElementsInstancedBaseVertex = NULL;
PFNGLMULTIDRAWELEMENTSBASEVERTEX_PROC* glpfMultiDrawElementsBaseVertex = NULL;
PFNGLPROVOKINGVERTEX_PROC* glpfProvokingVertex = NULL;
PFNGLFENCESYNC_PROC* glpfFenceSync = NULL;
PFNGLISSYNC_PROC* glpfIsSync = NULL;
PFNGLDELETESYNC_PROC* glpfDeleteSync = NULL;
PFNGLCLIENTWAITSYNC_PROC* glpfClientWaitSync = NULL;
PFNGLWAITSYNC_PROC* glpfWaitSync = NULL;
PFNGLGETINTEGER64V_PROC* glpfGetInteger64v = NULL;
PFNGLGETSYNCIV_PROC* glpfGetSynciv = NULL;
PFNGLGETINTEGER64I_V_PROC* glpfGetInteger64i_v = NULL;
PFNGLGETBUFFERPARAMETERI64V_PROC* glpfGetBufferParameteri64v = NULL;
PFNGLFRAMEBUFFERTEXTURE_PROC* glpfFramebufferTexture = NULL;
PFNGLTEXIMAGE2DMULTISAMPLE_PROC* glpfTexImage2DMultisample = NULL;
PFNGLTEXIMAGE3DMULTISAMPLE_PROC* glpfTexImage3DMultisample = NULL;
PFNGLGETMULTISAMPLEFV_PROC* glpfGetMultisamplefv = NULL;
PFNGLSAMPLEMASKI_PROC* glpfSampleMaski = NULL;

/* GL_VERSION_3_3 */

PFNGLBINDFRAGDATALOCATIONINDEXED_PROC* glpfBindFragDataLocationIndexed = NULL;
PFNGLGETFRAGDATAINDEX_PROC* glpfGetFragDataIndex = NULL;
PFNGLGENSAMPLERS_PROC* glpfGenSamplers = NULL;
PFNGLDELETESAMPLERS_PROC* glpfDeleteSamplers = NULL;
PFNGLISSAMPLER_PROC* glpfIsSampler = NULL;
PFNGLBINDSAMPLER_PROC* glpfBindSampler = NULL;
PFNGLSAMPLERPARAMETERI_PROC* glpfSamplerParameteri = NULL;
PFNGLSAMPLERPARAMETERIV_PROC* glpfSamplerParameteriv = NULL;
PFNGLSAMPLERPARAMETERF_PROC* glpfSamplerParameterf = NULL;
PFNGLSAMPLERPARAMETERFV_PROC* glpfSamplerParameterfv = NULL;
PFNGLSAMPLERPARAMETERIIV_PROC* glpfSamplerParameterIiv = NULL;
PFNGLSAMPLERPARAMETERIUIV_PROC* glpfSamplerParameterIuiv = NULL;
PFNGLGETSAMPLERPARAMETERIV_PROC* glpfGetSamplerParameteriv = NULL;
PFNGLGETSAMPLERPARAMETERIIV_PROC* glpfGetSamplerParameterIiv = NULL;
PFNGLGETSAMPLERPARAMETERFV_PROC* glpfGetSamplerParameterfv = NULL;
PFNGLGETSAMPLERPARAMETERIUIV_PROC* glpfGetSamplerParameterIuiv = NULL;
PFNGLQUERYCOUNTER_PROC* glpfQueryCounter = NULL;
PFNGLGETQUERYOBJECTI64V_PROC* glpfGetQueryObjecti64v = NULL;
PFNGLGETQUERYOBJECTUI64V_PROC* glpfGetQueryObjectui64v = NULL;
PFNGLVERTEXATTRIBDIVISOR_PROC* glpfVertexAttribDivisor = NULL;
PFNGLVERTEXATTRIBP1UI_PROC* glpfVertexAttribP1ui = NULL;
PFNGLVERTEXATTRIBP1UIV_PROC* glpfVertexAttribP1uiv = NULL;
PFNGLVERTEXATTRIBP2UI_PROC* glpfVertexAttribP2ui = NULL;
PFNGLVERTEXATTRIBP2UIV_PROC* glpfVertexAttribP2uiv = NULL;
PFNGLVERTEXATTRIBP3UI_PROC* glpfVertexAttribP3ui = NULL;
PFNGLVERTEXATTRIBP3UIV_PROC* glpfVertexAttribP3uiv = NULL;
PFNGLVERTEXATTRIBP4UI_PROC* glpfVertexAttribP4ui = NULL;
PFNGLVERTEXATTRIBP4UIV_PROC* glpfVertexAttribP4uiv = NULL;

/* GL_ARB_debug_output */

PFNGLDEBUGMESSAGECONTROLARB_PROC* glpfDebugMessageControlARB = NULL;
PFNGLDEBUGMESSAGEINSERTARB_PROC* glpfDebugMessageInsertARB = NULL;
PFNGLDEBUGMESSAGECALLBACKARB_PROC* glpfDebugMessageCallbackARB = NULL;
PFNGLGETDEBUGMESSAGELOGARB_PROC* glpfGetDebugMessageLogARB = NULL;

/* GL_ARB_direct_state_access */

PFNGLCREATEBUFFERSARB_PROC* glpfCreateBuffersARB = NULL;
PFNGLNAMEDBUFFERSTORAGEARB_PROC* glpfNamedBufferStorageARB = NULL;
PFNGLNAMEDBUFFERDATAARB_PROC* glpfNamedBufferDataARB = NULL;
PFNGLNAMEDBUFFERSUBDATAARB_PROC* glpfNamedBufferSubDataARB = NULL;
PFNGLCOPYNAMEDBUFFERSUBDATAARB_PROC* glpfCopyNamedBufferSubDataARB = NULL;
PFNGLCLEARNAMEDBUFFERDATAARB_PROC* glpfClearNamedBufferDataARB = NULL;
PFNGLCLEARNAMEDBUFFERSUBDATAARB_PROC* glpfClearNamedBufferSubDataARB = NULL;
PFNGLMAPNAMEDBUFFERARB_PROC* glpfMapNamedBufferARB = NULL;
PFNGLMAPNAMEDBUFFERRANGEARB_PROC* glpfMapNamedBufferRangeARB = NULL;
PFNGLUNMAPNAMEDBUFFERARB_PROC* glpfUnmapNamedBufferARB = NULL;
PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEARB_PROC* glpfFlushMappedNamedBufferRangeARB = NULL;
PFNGLGETNAMEDBUFFERPARAMETERIVARB_PROC* glpfGetNamedBufferParameterivARB = NULL;
PFNGLGETNAMEDBUFFERPARAMETERI64VARB_PROC* glpfGetNamedBufferParameteri64vARB = NULL;
PFNGLGETNAMEDBUFFERPOINTERVARB_PROC* glpfGetNamedBufferPointervARB = NULL;
PFNGLGETNAMEDBUFFERSUBDATAARB_PROC* glpfGetNamedBufferSubDataARB = NULL;

#ifdef __cplusplus
}
#endif
