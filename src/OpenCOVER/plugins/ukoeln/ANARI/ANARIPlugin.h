/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#pragma once

#include <cover/ui/Owner.h>
#include <cover/coVRPlugin.h>
#include <cover/coVRFileManager.h>
#include "coTransfuncEditor.h"
#include "Renderer.h"
#include "ColorMaps.h"

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

    static int loadMesh(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadMesh(const char *fileName, const char *);

    static int loadVolumeRAW(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadVolumeRAW(const char *fileName, const char *);

    static int loadFLASH(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadFLASH(const char *fileName, const char *);

    static int loadUMeshFile(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadUMeshFile(const char *fileName, const char *);

    static int loadUMeshScalars(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadUMeshScalars(const char *fileName, const char *);

    static int loadUMeshVTK(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadUMeshVTK(const char *fileName, const char *);

    static int loadPointCloud(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadPointCloud(const char *fileName, const char *);

    static int loadHDRI(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadHDRI(const char *fileName, const char *);

    static int loadXF(const char *fileName, osg::Group *loadParent, const char *);
    static int unloadXF(const char *fileName, const char *);


    ANARIPlugin();
   ~ANARIPlugin();

    bool init() override;

    void preFrame() override;
    void preDraw(osg::RenderInfo &) override;

    void expandBoundingSphere(osg::BoundingSphere &bs) override;

    void setTimestep(int ts) override;

    void addObject(const RenderObject *container, osg::Group *parent,
                   const RenderObject *geometry, const RenderObject *normals,
                   const RenderObject *colors, const RenderObject *texture) override;

    void removeObject(const char *objName, bool replaceFlag) override;

protected:
    ui::Menu *anariMenu = nullptr;
    ui::Menu *debugMenu = nullptr;
    ui::Menu *remoteMenu = nullptr;
    ui::Menu *rendererMenu = nullptr;
    ui::Group *rendererGroup = nullptr;
    ui::ButtonGroup *rendererButtonGroup = nullptr;
    std::vector<ui::Button *> rendererButtons;

    std::vector<ui::Element *> rendererUI;

    coTransfuncEditor *tfe = nullptr;
    util::ColorMapLibrary colorMaps;

    int previousRendererType = -1;
    int rendererType = 0;

private:
    Renderer::SP renderer = nullptr;

    void buildUI();
    void tearDownUI();
};


