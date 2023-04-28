/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef _GENERAL_GEOMETRY_PLUGIN_H
#define _GENERAL_GEOMETRY_PLUGIN_H

class ModuleFeedbackManager;
#include <PluginUtil/ModuleFeedbackPlugin.h>

class GeneralGeometryPlugin : public opencover::ModuleFeedbackPlugin
{
protected:
    virtual opencover::ModuleFeedbackManager *NewModuleFeedbackManager(const opencover::RenderObject *, opencover::coInteractor *, const opencover::RenderObject *, const char *);

public:
    // constructor
    GeneralGeometryPlugin();
    // destructor: deletes all items in the list
    virtual ~GeneralGeometryPlugin();

    virtual bool init();
    virtual void guiToRenderMsg(const grmsg::coGRMsg &msg) ;

    virtual void addNode(osg::Node *, const opencover::RenderObject * = NULL);
    virtual void removeObject(const char *objName, bool replaceFlag);
    virtual void addObject(const opencover::RenderObject *baseObj, osg::Group *parent, const opencover::RenderObject *, const opencover::RenderObject *, const opencover::RenderObject *, const opencover::RenderObject *);

    bool msgForGeneralGeometry(const char *moduleName);

    // message from gui
    void setColor(const char *objectName, int *color);
    //void setShader(const char *objectName, const char* shaderName, const char* paraFloat, const char* paraVec2, const char* paraVec3, const char* paraVec4, const char* paraInt, const char* paraBool, const char* paraMat2, const char* paraMat3, const char* paraMat4);
    void setTransparency(const char *objectName, float transparency);
    void setMaterial(const char *objectName, const int *ambient, const int *diffuse, const int *specular, float shininess, float transparency);
};
#endif
