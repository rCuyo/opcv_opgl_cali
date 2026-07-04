/* ============================================================================
 * glad.h - Loader minimalista de funciones OpenGL 3.3 Core Profile.
 *
 * Versión autocontenida (sin generador web ni Python) que expone únicamente
 * el subconjunto de la API de OpenGL 3.3 core que utiliza este proyecto.
 * Mantiene la misma interfaz que GLAD oficial:
 *
 *     if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { ... }
 *
 * Las funciones se resuelven en tiempo de ejecución mediante el callback
 * de carga (glfwGetProcAddress), por lo que no hace falta enlazar contra
 * opengl32/libGL directamente.
 * ==========================================================================*/
#ifndef GLAD_GLAD_H_
#define GLAD_GLAD_H_

/* Igual que el GLAD oficial: si ya se incluyó una cabecera OpenGL del
 * sistema los símbolos chocarían; y se definen sus guardas para que no
 * pueda incluirse después (p. ej. desde GLFW/glfw3.h). */
#if defined(__gl_h_) || defined(__GL_H__) || defined(_GL_H) || defined(__X_GL_H)
#  error "Se incluyo una cabecera OpenGL del sistema antes que glad.h"
#endif
#define __gl_h_
#define __GL_H__
#define _GL_H
#define __X_GL_H
/* Guardas adicionales de los headers de Apple (gl3.h / OpenGL.framework). */
#define __gl3_h_
#define __GL3_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Convención de llamada (Windows usa __stdcall) ----------------------*/
#ifndef APIENTRY
#  if defined(_WIN32) && !defined(__CYGWIN__)
#    define APIENTRY __stdcall
#  else
#    define APIENTRY
#  endif
#endif
#ifndef APIENTRYP
#  define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#  define GLAPI extern
#endif

/* ---- Tipos básicos de OpenGL --------------------------------------------*/
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef void           GLvoid;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef int            GLsizei;
typedef float          GLfloat;
typedef double         GLdouble;
typedef char           GLchar;
typedef ptrdiff_t      GLsizeiptr;
typedef ptrdiff_t      GLintptr;

/* ---- Constantes (subconjunto usado por el proyecto) ---------------------*/
#define GL_FALSE                          0
#define GL_TRUE                           1
#define GL_NO_ERROR                       0

#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_COLOR_BUFFER_BIT               0x00004000

#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005

#define GL_NEVER                          0x0200
#define GL_LESS                           0x0201
#define GL_EQUAL                          0x0202
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204
#define GL_ALWAYS                         0x0207

#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303

#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405
#define GL_FRONT_AND_BACK                 0x0408

#define GL_CW                             0x0900
#define GL_CCW                            0x0901

#define GL_CULL_FACE                      0x0B44
#define GL_DEPTH_TEST                     0x0B71
#define GL_BLEND                          0x0BE2
#define GL_LINE_SMOOTH                    0x0B20
#define GL_MULTISAMPLE                    0x809D

#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_PACK_ALIGNMENT                 0x0D05

#define GL_TEXTURE_2D                     0x0DE1

#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406

#define GL_VENDOR                         0x1F00
#define GL_RENDERER                       0x1F01
#define GL_VERSION                        0x1F02

#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_BGR                            0x80E0
#define GL_BGRA                           0x80E1
#define GL_RGB8                           0x8051
#define GL_RGBA8                          0x8058

#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_CLAMP_TO_EDGE                  0x812F

#define GL_TEXTURE0                       0x84C0

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_STREAM_DRAW                    0x88E0

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83
#define GL_INFO_LOG_LENGTH                0x8B84

#define GL_LINE                           0x1B01
#define GL_FILL                           0x1B02

/* ---- Punteros a funciones ------------------------------------------------
 * Cada función de OpenGL se declara como un puntero global (glad_glXxx) y
 * se expone mediante una macro con su nombre estándar (glXxx), igual que
 * en el GLAD generado oficialmente. */
typedef void (APIENTRYP PFNGLCLEARPROC)(GLbitfield mask);
typedef void (APIENTRYP PFNGLCLEARCOLORPROC)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
typedef void (APIENTRYP PFNGLENABLEPROC)(GLenum cap);
typedef void (APIENTRYP PFNGLDISABLEPROC)(GLenum cap);
typedef void (APIENTRYP PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLDEPTHFUNCPROC)(GLenum func);
typedef void (APIENTRYP PFNGLDEPTHMASKPROC)(GLboolean flag);
typedef void (APIENTRYP PFNGLBLENDFUNCPROC)(GLenum sfactor, GLenum dfactor);
typedef void (APIENTRYP PFNGLLINEWIDTHPROC)(GLfloat width);
typedef void (APIENTRYP PFNGLCULLFACEPROC)(GLenum mode);
typedef void (APIENTRYP PFNGLFRONTFACEPROC)(GLenum mode);
typedef void (APIENTRYP PFNGLPIXELSTOREIPROC)(GLenum pname, GLint param);
typedef GLenum (APIENTRYP PFNGLGETERRORPROC)(void);
typedef const GLubyte *(APIENTRYP PFNGLGETSTRINGPROC)(GLenum name);
typedef void (APIENTRYP PFNGLGETINTEGERVPROC)(GLenum pname, GLint *data);
typedef void (APIENTRYP PFNGLPOLYGONMODEPROC)(GLenum face, GLenum mode);

typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (APIENTRYP PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRYP PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (APIENTRYP PFNGLDRAWARRAYSPROC)(GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRYP PFNGLDRAWELEMENTSPROC)(GLenum mode, GLsizei count, GLenum type, const void *indices);

typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC)(GLenum type);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLDELETESHADERPROC)(GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (APIENTRYP PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (APIENTRYP PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRYP PFNGLUNIFORM3FVPROC)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORM4FVPROC)(GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

typedef void (APIENTRYP PFNGLGENTEXTURESPROC)(GLsizei n, GLuint *textures);
typedef void (APIENTRYP PFNGLBINDTEXTUREPROC)(GLenum target, GLuint texture);
typedef void (APIENTRYP PFNGLTEXIMAGE2DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRYP PFNGLTEXSUBIMAGE2DPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRYP PFNGLTEXPARAMETERIPROC)(GLenum target, GLenum pname, GLint param);
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (APIENTRYP PFNGLDELETETEXTURESPROC)(GLsizei n, const GLuint *textures);

/* ---- Declaración de los punteros globales --------------------------------*/
GLAPI PFNGLCLEARPROC                    glad_glClear;
GLAPI PFNGLCLEARCOLORPROC               glad_glClearColor;
GLAPI PFNGLENABLEPROC                   glad_glEnable;
GLAPI PFNGLDISABLEPROC                  glad_glDisable;
GLAPI PFNGLVIEWPORTPROC                 glad_glViewport;
GLAPI PFNGLDEPTHFUNCPROC                glad_glDepthFunc;
GLAPI PFNGLDEPTHMASKPROC                glad_glDepthMask;
GLAPI PFNGLBLENDFUNCPROC                glad_glBlendFunc;
GLAPI PFNGLLINEWIDTHPROC                glad_glLineWidth;
GLAPI PFNGLCULLFACEPROC                 glad_glCullFace;
GLAPI PFNGLFRONTFACEPROC                glad_glFrontFace;
GLAPI PFNGLPIXELSTOREIPROC              glad_glPixelStorei;
GLAPI PFNGLGETERRORPROC                 glad_glGetError;
GLAPI PFNGLGETSTRINGPROC                glad_glGetString;
GLAPI PFNGLGETINTEGERVPROC              glad_glGetIntegerv;
GLAPI PFNGLPOLYGONMODEPROC              glad_glPolygonMode;

GLAPI PFNGLGENVERTEXARRAYSPROC          glad_glGenVertexArrays;
GLAPI PFNGLBINDVERTEXARRAYPROC          glad_glBindVertexArray;
GLAPI PFNGLDELETEVERTEXARRAYSPROC       glad_glDeleteVertexArrays;
GLAPI PFNGLGENBUFFERSPROC               glad_glGenBuffers;
GLAPI PFNGLBINDBUFFERPROC               glad_glBindBuffer;
GLAPI PFNGLBUFFERDATAPROC               glad_glBufferData;
GLAPI PFNGLBUFFERSUBDATAPROC            glad_glBufferSubData;
GLAPI PFNGLDELETEBUFFERSPROC            glad_glDeleteBuffers;
GLAPI PFNGLVERTEXATTRIBPOINTERPROC      glad_glVertexAttribPointer;
GLAPI PFNGLENABLEVERTEXATTRIBARRAYPROC  glad_glEnableVertexAttribArray;
GLAPI PFNGLDRAWARRAYSPROC               glad_glDrawArrays;
GLAPI PFNGLDRAWELEMENTSPROC             glad_glDrawElements;

GLAPI PFNGLCREATESHADERPROC             glad_glCreateShader;
GLAPI PFNGLSHADERSOURCEPROC             glad_glShaderSource;
GLAPI PFNGLCOMPILESHADERPROC            glad_glCompileShader;
GLAPI PFNGLGETSHADERIVPROC              glad_glGetShaderiv;
GLAPI PFNGLGETSHADERINFOLOGPROC         glad_glGetShaderInfoLog;
GLAPI PFNGLDELETESHADERPROC             glad_glDeleteShader;
GLAPI PFNGLCREATEPROGRAMPROC            glad_glCreateProgram;
GLAPI PFNGLATTACHSHADERPROC             glad_glAttachShader;
GLAPI PFNGLLINKPROGRAMPROC              glad_glLinkProgram;
GLAPI PFNGLGETPROGRAMIVPROC             glad_glGetProgramiv;
GLAPI PFNGLGETPROGRAMINFOLOGPROC        glad_glGetProgramInfoLog;
GLAPI PFNGLUSEPROGRAMPROC               glad_glUseProgram;
GLAPI PFNGLDELETEPROGRAMPROC            glad_glDeleteProgram;
GLAPI PFNGLGETUNIFORMLOCATIONPROC       glad_glGetUniformLocation;
GLAPI PFNGLUNIFORM1IPROC                glad_glUniform1i;
GLAPI PFNGLUNIFORM1FPROC                glad_glUniform1f;
GLAPI PFNGLUNIFORM3FPROC                glad_glUniform3f;
GLAPI PFNGLUNIFORM3FVPROC               glad_glUniform3fv;
GLAPI PFNGLUNIFORM4FVPROC               glad_glUniform4fv;
GLAPI PFNGLUNIFORMMATRIX4FVPROC         glad_glUniformMatrix4fv;

GLAPI PFNGLGENTEXTURESPROC              glad_glGenTextures;
GLAPI PFNGLBINDTEXTUREPROC              glad_glBindTexture;
GLAPI PFNGLTEXIMAGE2DPROC               glad_glTexImage2D;
GLAPI PFNGLTEXSUBIMAGE2DPROC            glad_glTexSubImage2D;
GLAPI PFNGLTEXPARAMETERIPROC            glad_glTexParameteri;
GLAPI PFNGLACTIVETEXTUREPROC            glad_glActiveTexture;
GLAPI PFNGLDELETETEXTURESPROC           glad_glDeleteTextures;

/* ---- Macros con los nombres estándar -------------------------------------*/
#define glClear                    glad_glClear
#define glClearColor               glad_glClearColor
#define glEnable                   glad_glEnable
#define glDisable                  glad_glDisable
#define glViewport                 glad_glViewport
#define glDepthFunc                glad_glDepthFunc
#define glDepthMask                glad_glDepthMask
#define glBlendFunc                glad_glBlendFunc
#define glLineWidth                glad_glLineWidth
#define glCullFace                 glad_glCullFace
#define glFrontFace                glad_glFrontFace
#define glPixelStorei              glad_glPixelStorei
#define glGetError                 glad_glGetError
#define glGetString                glad_glGetString
#define glGetIntegerv              glad_glGetIntegerv
#define glPolygonMode              glad_glPolygonMode

#define glGenVertexArrays          glad_glGenVertexArrays
#define glBindVertexArray          glad_glBindVertexArray
#define glDeleteVertexArrays       glad_glDeleteVertexArrays
#define glGenBuffers               glad_glGenBuffers
#define glBindBuffer               glad_glBindBuffer
#define glBufferData               glad_glBufferData
#define glBufferSubData            glad_glBufferSubData
#define glDeleteBuffers            glad_glDeleteBuffers
#define glVertexAttribPointer      glad_glVertexAttribPointer
#define glEnableVertexAttribArray  glad_glEnableVertexAttribArray
#define glDrawArrays               glad_glDrawArrays
#define glDrawElements             glad_glDrawElements

#define glCreateShader             glad_glCreateShader
#define glShaderSource             glad_glShaderSource
#define glCompileShader            glad_glCompileShader
#define glGetShaderiv              glad_glGetShaderiv
#define glGetShaderInfoLog         glad_glGetShaderInfoLog
#define glDeleteShader             glad_glDeleteShader
#define glCreateProgram            glad_glCreateProgram
#define glAttachShader             glad_glAttachShader
#define glLinkProgram              glad_glLinkProgram
#define glGetProgramiv             glad_glGetProgramiv
#define glGetProgramInfoLog        glad_glGetProgramInfoLog
#define glUseProgram               glad_glUseProgram
#define glDeleteProgram            glad_glDeleteProgram
#define glGetUniformLocation       glad_glGetUniformLocation
#define glUniform1i                glad_glUniform1i
#define glUniform1f                glad_glUniform1f
#define glUniform3f                glad_glUniform3f
#define glUniform3fv               glad_glUniform3fv
#define glUniform4fv               glad_glUniform4fv
#define glUniformMatrix4fv         glad_glUniformMatrix4fv

#define glGenTextures              glad_glGenTextures
#define glBindTexture              glad_glBindTexture
#define glTexImage2D               glad_glTexImage2D
#define glTexSubImage2D            glad_glTexSubImage2D
#define glTexParameteri            glad_glTexParameteri
#define glActiveTexture            glad_glActiveTexture
#define glDeleteTextures           glad_glDeleteTextures

/* ---- Función de carga -----------------------------------------------------
 * Recibe un callback tipo glfwGetProcAddress y resuelve todos los punteros.
 * Devuelve 1 si todas las funciones esenciales se cargaron, 0 en caso
 * contrario. */
typedef void *(*GLADloadproc)(const char *name);
GLAPI int gladLoadGLLoader(GLADloadproc load);

#ifdef __cplusplus
}
#endif

#endif /* GLAD_GLAD_H_ */
