// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_renderer_gl.h"

#ifdef TB_RENDERER_GL
#include "tb_bitmap_fragment.h"
#include "tb_system.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO
uint32 dbg_bitmap_validations = 0;
#endif // TB_RUNTIME_DEBUG_INFO

// == Utilities ===================================================================================

#ifdef TB_RUNTIME_DEBUG_INFO
#define GLCALL(xxx) do {												\
		xxx;															\
		GLenum err = GL_NO_ERROR;										\
		while((err = glGetError()) != GL_NO_ERROR) {					\
			TBDebugPrint("%s:%i => gl error: %i", __FILE__, (int)__LINE__, (int)err); \
		} } while (0)
#else
#define GLCALL(xxx) do {} while (0)
#endif

#if !defined(TB_RENDERER_GLES_2) && !defined(TB_RENDERER_GL3)
static void Ortho2D(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top)
{
#ifdef TB_RENDERER_GLES_1
       glOrthof(left, right, bottom, top, -1.0, 1.0);
#else
       glOrtho(left, right, bottom, top, -1.0, 1.0);
#endif
}

#else

static void MakeOrtho(float * ortho, float l, float r, float b, float t, float n, float f)
{
	ortho[0] = 2 / (r - l);
	ortho[1] = 0;
	ortho[2]  = 0;
	ortho[3] = 0;

	ortho[4] = 0;
	ortho[5] = 2 / (t - b);
	ortho[6]  = 0;
	ortho[7] = 0;

	ortho[8] = 0;
	ortho[9] = 0;
	ortho[10] = -2 / (f - n);
	ortho[11] = 0;

	ortho[12] = -(r+l)/(r-l);
	ortho[13] = -(t+b)/(t-b);
	ortho[14] = -(f+n)/(f-n);
	ortho[15] = 1;
}
#endif

// == Batching ====================================================================================

GLuint g_current_texture = (GLuint)-1;
TBRendererBatcher::Batch *g_current_batch = 0;

void BindBitmap(TBBitmap *bitmap)
{
	GLuint texture = bitmap ? static_cast<TBBitmapGL*>(bitmap)->m_texture : 0;
	if (texture != g_current_texture)
	{
		g_current_texture = texture;
		GLCALL(glBindTexture(GL_TEXTURE_2D, g_current_texture));
	}
}

// == TBBitmapGL ==================================================================================

TBBitmapGL::TBBitmapGL(TBRendererGL *renderer)
	: m_renderer(renderer), m_w(0), m_h(0), m_texture(0)
{
}

TBBitmapGL::~TBBitmapGL()
{
	// Must flush and unbind before we delete the texture
	m_renderer->FlushBitmap(this);
	if (m_texture == g_current_texture)
		BindBitmap(nullptr);

	GLCALL(glDeleteTextures(1, &m_texture));
}

bool TBBitmapGL::Init(int width, int height, uint32 *data)
{
	assert(width == TBGetNearestPowerOfTwo(width));
	assert(height == TBGetNearestPowerOfTwo(height));

	m_w = width;
	m_h = height;

	GLCALL(glGenTextures(1, &m_texture));
	BindBitmap(this);
	GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

	SetData(data);

	return true;
}

void TBBitmapGL::SetData(uint32 *data)
{
	m_renderer->FlushBitmap(this);
	BindBitmap(this);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_w, m_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	TB_IF_DEBUG_SETTING(RENDER_BATCHES, dbg_bitmap_validations++);
}

// == TBRendererGL ================================================================================

TBRendererGL::TBRendererGL()
{
#if defined(TB_RENDERER_GLES_2) || defined(TB_RENDERER_GL3)

	GLchar vertexShaderString[] =  
#if defined(TB_RENDERER_GL3)
		"#version 150                    \n"
		"#define attribute in            \n"
		"#define varying out             \n"
#endif
		"attribute vec2 xy;              \n"
		"attribute vec2 uv;              \n"
		"attribute vec4 col;             \n"
		"uniform mat4 ortho;             \n"
		"uniform sampler2D tex;          \n"
		"varying vec2 uvo;               \n"
		"varying lowp vec4 color;        \n"
		"void main()                             \n"
		"{                                       \n"
		"  gl_Position = ortho * vec4(xy,0,1);   \n"
		"  uvo = uv;                             \n"
		"  color = col;                          \n"
		"}                                       \n";
	GLchar fragmentShaderString[] =
#if defined(TB_RENDERER_GL3)
		"#version 150                        \n"
		"#define varying in                  \n"
		"out vec4 fragData[1];               \n"
		"#define gl_FragColor fragData[0]    \n"
		"#define texture2D texture                     \n"
#endif
		"precision mediump float;                      \n"
		"varying vec2 uvo;                             \n"
		"varying lowp vec4 color;                      \n"
		"uniform sampler2D tex;                        \n"
		"void main()                                   \n"
		"{                                             \n"
		"  gl_FragColor = color * texture2D(tex, uvo); \n"
		"}                                             \n";

	GLuint vertexShader;
	GLuint fragmentShader;
	GLint linked;

	vertexShader = LoadShader(GL_VERTEX_SHADER, vertexShaderString);
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fragmentShaderString);

	m_program = glCreateProgram();
	if (m_program == 0)
	{
		TBDebugPrint("glCreateProgram failed.\n", 0);
		return;
	}

	glAttachShader(m_program, vertexShader);
	glAttachShader(m_program, fragmentShader);
	glBindAttribLocation(m_program, 0, "xy");
	glBindAttribLocation(m_program, 1, "uv");
	glBindAttribLocation(m_program, 2, "color");
	glLinkProgram(m_program);
	glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLint infoLen = 0;
		glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1)
		{
			char * infoLog = (char *)malloc(sizeof(char) * infoLen);
			glGetProgramInfoLog(m_program, infoLen, NULL, infoLog);
			TBDebugPrint("Error linking program:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteProgram(m_program);
		TBDebugPrint("glLinkProgram failed.\n", 0);
		return;
	}

	m_orthoLoc = glGetUniformLocation(m_program, "ortho");
	m_texLoc = glGetUniformLocation(m_program, "tex");

	GLCALL(glGenVertexArrays(1, &m_vao));
	GLCALL(glBindVertexArray(m_vao));

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	GLCALL(glGenBuffers(1, &m_vbo));
	GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
	GLCALL(glBufferData(GL_ARRAY_BUFFER, sizeof(batch.vertex), (void *)&batch.vertex[0], GL_DYNAMIC_DRAW));
#endif
}

#if defined(TB_RENDERER_GLES_2) || defined(TB_RENDERER_GL3)
GLuint TBRendererGL::LoadShader(GLenum type, const GLchar *shaderSrc)
{
	GLuint shader;
	GLint compiled;

	shader = glCreateShader(type);
	if (shader == 0)
		return 0;

	glShaderSource(shader, 1, &shaderSrc, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1)
		{
			char * infoLog = (char *)malloc(sizeof(char) * infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			TBDebugPrint("Error compiling shader:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
#endif

void TBRendererGL::BeginPaint(int render_target_w, int render_target_h)
{
#ifdef TB_RUNTIME_DEBUG_INFO
	dbg_bitmap_validations = 0;
#endif

	TBRendererBatcher::BeginPaint(render_target_w, render_target_h);

	g_current_texture = (GLuint)-1;
	g_current_batch = nullptr;

#if defined(TB_RENDERER_GLES_2) || defined(TB_RENDERER_GL3)
	GLCALL(glBindVertexArray(m_vao));
	GLCALL(glUseProgram(m_program));
	static float ortho[16];
	MakeOrtho(ortho, 0, (GLfloat)render_target_w, (GLfloat)render_target_h, 0, -1.0, 1.0);
	GLCALL(glUniformMatrix4fv(m_orthoLoc, 1, GL_FALSE, ortho));
	//GLCALL(glUniform1i(m_texLoc, GL_TEXTURE0));
	//GLCALL(glActiveTexture(GL_TEXTURE0));
#else
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	Ortho2D(0, (GLfloat)render_target_w, (GLfloat)render_target_h, 0);
	glMatrixMode(GL_MODELVIEW);
#endif

	glViewport(0, 0, render_target_w, render_target_h);
	glScissor(0, 0, render_target_w, render_target_h);

	GLCALL(glEnable(GL_BLEND));
#if !defined(TB_RENDERER_GLES_2) && !defined(TB_RENDERER_GL3)
	GLCALL(glEnable(GL_TEXTURE_2D));
#endif
	GLCALL(glDisable(GL_DEPTH_TEST));
	GLCALL(glEnable(GL_SCISSOR_TEST));
	GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

#if !defined(TB_RENDERER_GLES_2) && !defined(TB_RENDERER_GL3)
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
#endif
}

void TBRendererGL::EndPaint()
{
	TBRendererBatcher::EndPaint();

#ifdef TB_RUNTIME_DEBUG_INFO
	if (TB_DEBUG_SETTING(RENDER_BATCHES))
		TBDebugPrint("Frame caused %d bitmap validations.\n", dbg_bitmap_validations);
#endif // TB_RUNTIME_DEBUG_INFO
}

TBBitmap *TBRendererGL::CreateBitmap(int width, int height, uint32 *data)
{
	TBBitmapGL *bitmap = new TBBitmapGL(this);
	if (!bitmap || !bitmap->Init(width, height, data))
	{
		delete bitmap;
		return nullptr;
	}
	return bitmap;
}

void TBRendererGL::RenderBatch(Batch *batch)
{
	// Bind texture and array pointers
	BindBitmap(batch->bitmap);

	if (g_current_batch != batch)
	{
#if defined(TB_RENDERER_GLES_2) || defined(TB_RENDERER_GL3)
		GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
		GLCALL(glEnableVertexAttribArray(0));
		GLCALL(glEnableVertexAttribArray(1));
		GLCALL(glEnableVertexAttribArray(2));
		GLCALL(glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, sizeof(Vertex), &((Vertex *)NULL)->x));
		GLCALL(glVertexAttribPointer(1, 2, GL_FLOAT,         GL_FALSE, sizeof(Vertex), &((Vertex *)NULL)->u));
		GLCALL(glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Vertex), &((Vertex *)NULL)->col));
		g_current_batch = batch;
#else
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), (void *) &batch->vertex[0].r);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), (void *) &batch->vertex[0].u);
		glVertexPointer(2, GL_FLOAT, sizeof(Vertex), (void *) &batch->vertex[0].x);
		g_current_batch = batch;
#endif
	}

	// Flush
#if defined(TB_RENDERER_GLES_2) || defined(TB_RENDERER_GL3)
	GLCALL(glBufferSubData(GL_ARRAY_BUFFER, 0, batch->vertex_count * sizeof(Vertex), (void *)&batch->vertex[0]));
#endif
	GLCALL(glDrawArrays(GL_TRIANGLES, 0, batch->vertex_count));
}

void TBRendererGL::SetClipRect(const TBRect &rect)
{
	GLCALL(glScissor(m_clip_rect.x, m_screen_rect.h - (m_clip_rect.y + m_clip_rect.h), m_clip_rect.w, m_clip_rect.h));
}

} // namespace tb

#endif // TB_RENDERER_GL
