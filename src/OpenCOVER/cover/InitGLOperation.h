#ifndef INITGLOPERATION_H
#define INITGLOPERATION_H

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

#include <osgViewer/ViewerEventHandlers>

namespace opencover {

class InitGLOperation: public osg::GraphicsOperation
{
public:
    InitGLOperation();
    virtual void operator ()(osg::GraphicsContext* gc);

    bool boundSwapBarrier() const;
    osg::GLExtensions *getExtensions(int contextId = 0) const;

private:
    mutable OpenThreads::Mutex m_mutex;
    static void  debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                    GLsizei length, const GLchar *message, void *userData);
    static void  debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                    GLsizei length, const GLchar *message, const void *userData);

    struct DebugCallbackData
    {
        int contextId;
        int debugLevel;
        bool abortOnError;
    };
    DebugCallbackData m_callbackData;

    bool m_boundSwapBarrier = false;

    std::vector<osg::ref_ptr<osg::GLExtensions>> m_extensions;
};

}
#endif
