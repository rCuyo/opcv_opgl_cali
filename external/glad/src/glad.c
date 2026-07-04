/* ============================================================================
 * glad.c - Implementación del loader minimalista de OpenGL 3.3 Core.
 *
 * Define los punteros globales declarados en glad.h y los resuelve en
 * gladLoadGLLoader() usando el callback proporcionado (glfwGetProcAddress).
 * ==========================================================================*/
#include <glad/glad.h>

PFNGLCLEARPROC                    glad_glClear                   = 0;
PFNGLCLEARCOLORPROC               glad_glClearColor              = 0;
PFNGLENABLEPROC                   glad_glEnable                  = 0;
PFNGLDISABLEPROC                  glad_glDisable                 = 0;
PFNGLVIEWPORTPROC                 glad_glViewport                = 0;
PFNGLDEPTHFUNCPROC                glad_glDepthFunc               = 0;
PFNGLDEPTHMASKPROC                glad_glDepthMask               = 0;
PFNGLBLENDFUNCPROC                glad_glBlendFunc               = 0;
PFNGLLINEWIDTHPROC                glad_glLineWidth               = 0;
PFNGLCULLFACEPROC                 glad_glCullFace                = 0;
PFNGLFRONTFACEPROC                glad_glFrontFace               = 0;
PFNGLPIXELSTOREIPROC              glad_glPixelStorei             = 0;
PFNGLGETERRORPROC                 glad_glGetError                = 0;
PFNGLGETSTRINGPROC                glad_glGetString               = 0;
PFNGLGETINTEGERVPROC              glad_glGetIntegerv             = 0;
PFNGLPOLYGONMODEPROC              glad_glPolygonMode             = 0;

PFNGLGENVERTEXARRAYSPROC          glad_glGenVertexArrays         = 0;
PFNGLBINDVERTEXARRAYPROC          glad_glBindVertexArray         = 0;
PFNGLDELETEVERTEXARRAYSPROC       glad_glDeleteVertexArrays      = 0;
PFNGLGENBUFFERSPROC               glad_glGenBuffers              = 0;
PFNGLBINDBUFFERPROC               glad_glBindBuffer              = 0;
PFNGLBUFFERDATAPROC               glad_glBufferData              = 0;
PFNGLBUFFERSUBDATAPROC            glad_glBufferSubData           = 0;
PFNGLDELETEBUFFERSPROC            glad_glDeleteBuffers           = 0;
PFNGLVERTEXATTRIBPOINTERPROC      glad_glVertexAttribPointer     = 0;
PFNGLENABLEVERTEXATTRIBARRAYPROC  glad_glEnableVertexAttribArray = 0;
PFNGLDRAWARRAYSPROC               glad_glDrawArrays              = 0;
PFNGLDRAWELEMENTSPROC             glad_glDrawElements            = 0;

PFNGLCREATESHADERPROC             glad_glCreateShader            = 0;
PFNGLSHADERSOURCEPROC             glad_glShaderSource            = 0;
PFNGLCOMPILESHADERPROC            glad_glCompileShader           = 0;
PFNGLGETSHADERIVPROC              glad_glGetShaderiv             = 0;
PFNGLGETSHADERINFOLOGPROC         glad_glGetShaderInfoLog        = 0;
PFNGLDELETESHADERPROC             glad_glDeleteShader            = 0;
PFNGLCREATEPROGRAMPROC            glad_glCreateProgram           = 0;
PFNGLATTACHSHADERPROC             glad_glAttachShader            = 0;
PFNGLLINKPROGRAMPROC              glad_glLinkProgram             = 0;
PFNGLGETPROGRAMIVPROC             glad_glGetProgramiv            = 0;
PFNGLGETPROGRAMINFOLOGPROC        glad_glGetProgramInfoLog       = 0;
PFNGLUSEPROGRAMPROC               glad_glUseProgram              = 0;
PFNGLDELETEPROGRAMPROC            glad_glDeleteProgram           = 0;
PFNGLGETUNIFORMLOCATIONPROC       glad_glGetUniformLocation      = 0;
PFNGLUNIFORM1IPROC                glad_glUniform1i               = 0;
PFNGLUNIFORM1FPROC                glad_glUniform1f               = 0;
PFNGLUNIFORM3FPROC                glad_glUniform3f               = 0;
PFNGLUNIFORM3FVPROC               glad_glUniform3fv              = 0;
PFNGLUNIFORM4FVPROC               glad_glUniform4fv              = 0;
PFNGLUNIFORMMATRIX4FVPROC         glad_glUniformMatrix4fv        = 0;

PFNGLGENTEXTURESPROC              glad_glGenTextures             = 0;
PFNGLBINDTEXTUREPROC              glad_glBindTexture             = 0;
PFNGLTEXIMAGE2DPROC               glad_glTexImage2D              = 0;
PFNGLTEXSUBIMAGE2DPROC            glad_glTexSubImage2D           = 0;
PFNGLTEXPARAMETERIPROC            glad_glTexParameteri           = 0;
PFNGLACTIVETEXTUREPROC            glad_glActiveTexture           = 0;
PFNGLDELETETEXTURESPROC           glad_glDeleteTextures          = 0;

/* Macro auxiliar: resuelve una función y acumula fallos en 'ok'. */
#define GLAD_LOAD(type, name)                       \
    do {                                            \
        glad_##name = (type)load(#name);            \
        if (!glad_##name) ok = 0;                   \
    } while (0)

int gladLoadGLLoader(GLADloadproc load)
{
    int ok = 1;
    if (!load) return 0;

    GLAD_LOAD(PFNGLCLEARPROC,                   glClear);
    GLAD_LOAD(PFNGLCLEARCOLORPROC,              glClearColor);
    GLAD_LOAD(PFNGLENABLEPROC,                  glEnable);
    GLAD_LOAD(PFNGLDISABLEPROC,                 glDisable);
    GLAD_LOAD(PFNGLVIEWPORTPROC,                glViewport);
    GLAD_LOAD(PFNGLDEPTHFUNCPROC,               glDepthFunc);
    GLAD_LOAD(PFNGLDEPTHMASKPROC,               glDepthMask);
    GLAD_LOAD(PFNGLBLENDFUNCPROC,               glBlendFunc);
    GLAD_LOAD(PFNGLLINEWIDTHPROC,               glLineWidth);
    GLAD_LOAD(PFNGLCULLFACEPROC,                glCullFace);
    GLAD_LOAD(PFNGLFRONTFACEPROC,               glFrontFace);
    GLAD_LOAD(PFNGLPIXELSTOREIPROC,             glPixelStorei);
    GLAD_LOAD(PFNGLGETERRORPROC,                glGetError);
    GLAD_LOAD(PFNGLGETSTRINGPROC,               glGetString);
    GLAD_LOAD(PFNGLGETINTEGERVPROC,             glGetIntegerv);
    GLAD_LOAD(PFNGLPOLYGONMODEPROC,             glPolygonMode);

    GLAD_LOAD(PFNGLGENVERTEXARRAYSPROC,         glGenVertexArrays);
    GLAD_LOAD(PFNGLBINDVERTEXARRAYPROC,         glBindVertexArray);
    GLAD_LOAD(PFNGLDELETEVERTEXARRAYSPROC,      glDeleteVertexArrays);
    GLAD_LOAD(PFNGLGENBUFFERSPROC,              glGenBuffers);
    GLAD_LOAD(PFNGLBINDBUFFERPROC,              glBindBuffer);
    GLAD_LOAD(PFNGLBUFFERDATAPROC,              glBufferData);
    GLAD_LOAD(PFNGLBUFFERSUBDATAPROC,           glBufferSubData);
    GLAD_LOAD(PFNGLDELETEBUFFERSPROC,           glDeleteBuffers);
    GLAD_LOAD(PFNGLVERTEXATTRIBPOINTERPROC,     glVertexAttribPointer);
    GLAD_LOAD(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    GLAD_LOAD(PFNGLDRAWARRAYSPROC,              glDrawArrays);
    GLAD_LOAD(PFNGLDRAWELEMENTSPROC,            glDrawElements);

    GLAD_LOAD(PFNGLCREATESHADERPROC,            glCreateShader);
    GLAD_LOAD(PFNGLSHADERSOURCEPROC,            glShaderSource);
    GLAD_LOAD(PFNGLCOMPILESHADERPROC,           glCompileShader);
    GLAD_LOAD(PFNGLGETSHADERIVPROC,             glGetShaderiv);
    GLAD_LOAD(PFNGLGETSHADERINFOLOGPROC,        glGetShaderInfoLog);
    GLAD_LOAD(PFNGLDELETESHADERPROC,            glDeleteShader);
    GLAD_LOAD(PFNGLCREATEPROGRAMPROC,           glCreateProgram);
    GLAD_LOAD(PFNGLATTACHSHADERPROC,            glAttachShader);
    GLAD_LOAD(PFNGLLINKPROGRAMPROC,             glLinkProgram);
    GLAD_LOAD(PFNGLGETPROGRAMIVPROC,            glGetProgramiv);
    GLAD_LOAD(PFNGLGETPROGRAMINFOLOGPROC,       glGetProgramInfoLog);
    GLAD_LOAD(PFNGLUSEPROGRAMPROC,              glUseProgram);
    GLAD_LOAD(PFNGLDELETEPROGRAMPROC,           glDeleteProgram);
    GLAD_LOAD(PFNGLGETUNIFORMLOCATIONPROC,      glGetUniformLocation);
    GLAD_LOAD(PFNGLUNIFORM1IPROC,               glUniform1i);
    GLAD_LOAD(PFNGLUNIFORM1FPROC,               glUniform1f);
    GLAD_LOAD(PFNGLUNIFORM3FPROC,               glUniform3f);
    GLAD_LOAD(PFNGLUNIFORM3FVPROC,              glUniform3fv);
    GLAD_LOAD(PFNGLUNIFORM4FVPROC,              glUniform4fv);
    GLAD_LOAD(PFNGLUNIFORMMATRIX4FVPROC,        glUniformMatrix4fv);

    GLAD_LOAD(PFNGLGENTEXTURESPROC,             glGenTextures);
    GLAD_LOAD(PFNGLBINDTEXTUREPROC,             glBindTexture);
    GLAD_LOAD(PFNGLTEXIMAGE2DPROC,              glTexImage2D);
    GLAD_LOAD(PFNGLTEXSUBIMAGE2DPROC,           glTexSubImage2D);
    GLAD_LOAD(PFNGLTEXPARAMETERIPROC,           glTexParameteri);
    GLAD_LOAD(PFNGLACTIVETEXTUREPROC,           glActiveTexture);
    GLAD_LOAD(PFNGLDELETETEXTURESPROC,          glDeleteTextures);

    return ok;
}
