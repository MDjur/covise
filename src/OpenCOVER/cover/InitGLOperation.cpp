/* based on https://github.com/ThermalPixel/osgdemos/osgdebug

   Copyright (c) 2014, Andreas Klein
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met: 

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer. 
   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution. 

      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
      ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
      WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
      DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
      ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
      (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
      LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
      ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
      SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

      The views and conclusions contained in the software and documentation are those
      of the authors and should not be interpreted as representing official policies, 
      either expressed or implied, of the FreeBSD Project.
*/

#include <iostream>
#include <sstream>

#include <GL/glew.h>
#ifdef USE_X11
#include <GL/glxew.h>
#include <osgViewer/api/X11/GraphicsWindowX11>
#undef Status
#endif

#include <osg/GL>
#include <osg/GLExtensions>
#include <osg/GL2Extensions>
#include <osg/Notify>

#include <config/CoviseConfig.h>
#include "coVRConfig.h"
#include "coVRPluginSupport.h"

#include "InitGLOperation.h"

#ifndef GL_VERSION_4_3
typedef void (GL_APIENTRY *GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
#endif
typedef void (GL_APIENTRY *GLDebugMessageControlPROC)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (GL_APIENTRY *GLDebugMessageCallbackPROC)(GLDEBUGPROC callback, void *userParam);

#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH 0x8243
#define GL_DEBUG_CALLBACK_FUNCTION 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM 0x8245
#define GL_DEBUG_SOURCE_API 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY 0x8249
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
#define GL_DEBUG_SOURCE_OTHER 0x824B
#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#define GL_DEBUG_TYPE_PORTABILITY 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#define GL_DEBUG_TYPE_OTHER 0x8251
#define GL_MAX_DEBUG_MESSAGE_LENGTH 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES 0x9144
#define GL_DEBUG_LOGGED_MESSAGES 0x9145
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#define GL_DEBUG_SEVERITY_LOW 0x9148
#define GL_DEBUG_TYPE_MARKER 0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP 0x8269
#define GL_DEBUG_TYPE_POP_GROUP 0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH 0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH 0x826D
#define GL_BUFFER 0x82E0
#define GL_SHADER 0x82E1
#define GL_PROGRAM 0x82E2
#define GL_QUERY 0x82E3
#define GL_PROGRAM_PIPELINE 0x82E4
#define GL_SAMPLER 0x82E6
#define GL_DISPLAY_LIST 0x82E7
#define GL_MAX_LABEL_LENGTH 0x82E8
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002

namespace opencover {

InitGLOperation::InitGLOperation()
: osg::GraphicsOperation("InitGLOperation", false)
{
}

void InitGLOperation::operator()(osg::GraphicsContext* gc)
{
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "glewInit() failed" << std::endl;
        return;
    }

#if defined(USE_X11) && defined(glxewInit)
    if (glxewInit() != GLEW_OK)
    {
        std::cerr << "glxewInit() failed" << std::endl;
    }
#endif

#define PRINT_STRING(s) std::cerr << "GL_" #s << ": " << glGetString(GL_ ## s) << std::endl
    if (cover->debugLevel(2))
    {
        PRINT_STRING(RENDERER);
        PRINT_STRING(VENDOR);
        PRINT_STRING(VERSION);
    }
    if (cover->debugLevel(3))
    {
        PRINT_STRING(EXTENSIONS);
    }
#undef PRINT_STRING

    const bool glDebug = covise::coCoviseConfig::isOn("COVER.GLDebug", false);
    bool glDebugLevelExists = false;
    int glDebugLevel = covise::coCoviseConfig::getInt("level", "COVER.GLDebug", 1, &glDebugLevelExists);
    bool abortOnErrorExists = false;
    bool abortOnError = covise::coCoviseConfig::isOn("abortOnError", "COVER.GLDebug", false, &abortOnErrorExists);

    if (glDebug || (glDebugLevelExists && glDebugLevel > 0) || abortOnError) {
        std::cerr << "VRViewer: enabling GL debugging" << std::endl;

        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(m_mutex);

        int contextId = gc->getState()->getContextID();

        //create the extensions
        GLDebugMessageControlPROC glDebugMessageControl = NULL;
        GLDebugMessageCallbackPROC glDebugMessageCallback = NULL;

        std::string ext;
        if(osg::isGLExtensionSupported(contextId, "GL_KHR_debug"))
        {
            ext = "GL_KHR_debug";
            osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallback");
            osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControl");

        }
        else if(osg::isGLExtensionSupported(contextId, "GL_ARB_debug_output"))
        {
            ext = "GL_ARB_debug_output";
            osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallbackARB");
            osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControlARB");
        }
        else if(osg::isGLExtensionSupported(contextId, "GL_AMD_debug_output"))
        {
            ext = "GL_AMD_debug_output";
            osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallbackAMD");
            osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControlAMD");
        }
        else
        {
            std::cerr << "enableGLDebugExtension: no supported GL extension found for context " << contextId
                      << std::endl;
        }

        if (!ext.empty())
        {
            if (glDebugMessageCallback == NULL || glDebugMessageControl == NULL)
            {
                std::cerr << "enableGLDebugExtension: did not find required function for GL extension " << ext
                          << " for context " << contextId << std::endl;
            }
            else
            {
                m_callbackData = {contextId, glDebugLevel, abortOnError};
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
                glDebugMessageCallback(InitGLOperation::debugCallback, reinterpret_cast<void *>(&m_callbackData));

                std::cerr << "enableGLDebugExtension: " << ext << " enabled on context " << contextId << std::endl;
            }
        }
    }

    bool sRGB = covise::coCoviseConfig::isOn("COVER.FramebufferSRGB", false);

    if (sRGB)
    {
        std::cerr << "Enable GL_FRAMEBUFFER_SRGB" << std::endl;
        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    // setup swap groups and swap barriers
#ifdef USE_X11
    if (glXJoinSwapGroupNV && glXBindSwapBarrierNV)
    {
        auto &conf = *opencover::coVRConfig::instance();
        for(int i=0; i<conf.numWindows();i++)
        {
            if(conf.windows[i].context == gc)
            {
                osgViewer::GraphicsWindowX11 *window = dynamic_cast<osgViewer::GraphicsWindowX11 *>(gc);
                if (!window)
                    continue;
                if(conf.windows[i].swapGroup > 0)
                {
                    if (!glXJoinSwapGroupNV(window->getDisplayToUse(),window->getWindow(),conf.windows[i].swapGroup))
                    {
                        std::cerr << "Failed to join GL swap group " << conf.windows[i].swapGroup << std::endl;
                    }
                    else if(conf.windows[i].swapBarrier > 0)
                    {
                        if (!glXBindSwapBarrierNV(window->getDisplayToUse(),conf.windows[i].swapGroup,conf.windows[i].swapBarrier))
                        {
                            std::cerr << "Failed to bind GL swap group " << conf.windows[i].swapGroup << " to barrier " << conf.windows[i].swapBarrier << std::endl;
                        }
                        else
                        {
                            m_boundSwapBarrier = true;
                        }
                    }
                }
            }
        }
    }
#endif

    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(m_mutex);
    int contextId = gc->getState()->getContextID();
    osg::GLExtensions *glext = gc->getState()->get<osg::GLExtensions>();
    if (m_extensions.size() <= contextId)
    {
        m_extensions.resize(contextId + 1);
    }
    m_extensions[contextId] = glext;
}

bool InitGLOperation::boundSwapBarrier() const
{
    return m_boundSwapBarrier;
}

osg::GLExtensions *InitGLOperation::getExtensions(int contextId) const
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(m_mutex);
    if (m_extensions.size() <= contextId)
    {
        return nullptr;
    }
    return m_extensions[contextId].get();
}

void InitGLOperation::debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                    GLsizei length, const GLchar *message, void *userData)
{
    debugCallback(source, type, id, severity, length, message, const_cast<const void *>(userData));
}

void InitGLOperation::debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                    GLsizei length, const GLchar *message, const void *userData)
{
    InitGLOperation::DebugCallbackData cbdata = *reinterpret_cast<const InitGLOperation::DebugCallbackData *>(userData);
    const int ctxId = cbdata.contextId;
    std::string srcStr = "UNDEFINED";
    switch(source)

    {
    case GL_DEBUG_SOURCE_API:             srcStr = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   srcStr = "WINDOW_SYSTEM"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: srcStr = "SHADER_COMPILER"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     srcStr = "THIRD_PARTY"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     srcStr = "APPLICATION"; break;
    case GL_DEBUG_SOURCE_OTHER:           srcStr = "OTHER"; break;
    }

    std::string typeStr = "UNDEFINED";
    switch(type)
    {

    case GL_DEBUG_TYPE_ERROR:
        //	__debugbreak();
        typeStr = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeStr = "OTHER";
        break;
    }

    std::string severityStr = "";
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        severityStr = "HIGH";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityStr = "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        severityStr = "LOW";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        severityStr = "NOTIFICATION";
        break;
    }

    bool printMsg = severity==GL_DEBUG_SEVERITY_HIGH;
    if (cbdata.debugLevel >= 1 && severity==GL_DEBUG_SEVERITY_MEDIUM)
        printMsg = true;
    if (cbdata.debugLevel >= 2 && severity==GL_DEBUG_SEVERITY_LOW)
    {
        // filter warnings about non full-screen clears
        if (type != GL_DEBUG_TYPE_OTHER && source != GL_DEBUG_SOURCE_API)
            printMsg = true;
        else if (cbdata.debugLevel >= 3)
            printMsg = true;
    }
    if (cbdata.debugLevel >= 3 && severity==GL_DEBUG_SEVERITY_NOTIFICATION)
        printMsg = true;

    std::stringstream msg;
    msg << "GL ctx " << ctxId << ": " << typeStr << " (" << severityStr << ")" <<  " [" << srcStr <<"]: " << std::string(message, length);

    bool print = (cbdata.debugLevel >= 1 && type == GL_DEBUG_TYPE_ERROR)
                 || (cbdata.debugLevel >= 2 && type == GL_DEBUG_TYPE_PERFORMANCE)
                 ||  cbdata.debugLevel >= 3;
    if (cbdata.abortOnError && type==GL_DEBUG_TYPE_ERROR)
        print = true;

    if (print || printMsg)
        std::cerr << msg.str() << std::endl;

    if (cbdata.abortOnError && type==GL_DEBUG_TYPE_ERROR)
    {
        abort();
    }
}

}
