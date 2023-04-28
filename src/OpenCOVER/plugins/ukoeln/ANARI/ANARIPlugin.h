/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#pragma once

#include <cover/ui/Owner.h>
#include <cover/coVRPlugin.h>
#include <cover/coVRFileManager.h>
#include "Renderer.h"

namespace opencover {
namespace ui {
class Element;
class Group;
class Menu;
class Button;
class ButtonGroup;
class Slider;
}
}

using namespace opencover;

class ANARIPlugin : public coVRPlugin, public ui::Owner
{

public:
    static ANARIPlugin *plugin;
    static ANARIPlugin *instance();

    static int loadScene(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadScene(const char *fileName, const char *);

    static int loadVolumeRAW(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadVolumeRAW(const char *fileName, const char *);

    ANARIPlugin();
   ~ANARIPlugin();

    bool init();

    void preDraw(osg::RenderInfo &info);

    void expandBoundingSphere(osg::BoundingSphere &bs);

    void addObject(const RenderObject *container, osg::Group *parent,
                   const RenderObject *geometry, const RenderObject *normals,
                   const RenderObject *colors, const RenderObject *texture);

    void removeObject(const char *objName, bool replaceFlag);

protected:
    ui::Menu *anariMenu = nullptr;
    ui::Slider *sppSlider = nullptr;
    ui::Menu *rendererMenu = nullptr;
    ui::Group *rendererGroup = nullptr;
    ui::ButtonGroup *rendererButtonGroup = nullptr;
    std::vector<ui::Button *> rendererButtons;

private:
    Renderer::SP renderer = nullptr;

};


