/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

//-*-c++-*-

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/Texture2D>
#include <OpenVRUI/coAction.h>
#include <OpenVRUI/coCombinedButtonInteraction.h>
#include <OpenVRUI/coFlatPanelGeometry.h>
#include <OpenVRUI/osg/OSGVruiTransformNode.h>
#include <OpenVRUI/sginterface/vruiHit.h>
#include <cover/coIntersection.h>
#include "coTransfuncEditor.h"

using namespace vrui;
using namespace opencover;

class Canvas : public coAction,
               public coUIElement
{
public:
    Canvas() : dcs(new OSGVruiTransformNode(new osg::MatrixTransform()))
    {
        opacity.data.resize(width);
        for (int x=0; x<width; ++x) {
            opacity.data[x] = x/(width-1.f);
        }

        dcs->getNodePtr()->asGroup()->addChild(createGeode());
        coIntersection::getIntersectorForAction("coAction")->add(dcs, this);
        interactionA = new coCombinedButtonInteraction(coInteraction::ButtonA, "TFE Canvas", coInteraction::Menu);
    }

   ~Canvas()
    {
        delete interactionA;

        coIntersection::getIntersectorForAction("coAction")->remove(this);
        dcs->removeAllChildren();
        dcs->removeAllParents();
        delete dcs;
    }

    void update()
    {
        if (unregister) {
            if (interactionA->isRegistered()) {
                coInteractionManager::the()->unregisterInteraction(interactionA);
            }
            unregister = false;
        }

        if (opacity.updated) {
            if (opacity.updateFunc) opacity.updateFunc(opacity.data.data(),
                                                       opacity.data.size(),
                                                       opacity.userData);
            opacity.updated = false;
        }
    }

    int hit(vruiHit *hit)
    {
        if (!interactionA->isRegistered()) {
            coInteractionManager::the()->registerInteraction(interactionA);
            interactionA->setHitByMouse(hit->isMouseHit());
        }

        if (interactionA->isRunning()) {
            int brushSize=10;
            int x = hit->getLocalIntersectionPoint()[0];
            int y = hit->getLocalIntersectionPoint()[1];
            for (int i=-brushSize/2;i!=brushSize/2;++i) {
                int xi = std::max(0,std::min(int(width)-1,x+i));
                opacity.data[xi] = y/(height-1.f);
                opacity.updated = true;
            }
            updateAlphaTextureData();
            auto image = alphaTexture->getImage();
            if (!image) {
                image = new osg::Image;
                alphaTexture->setImage(image);
            }
            image->setImage(width, height, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE,
                            alphaTextureData.data(), osg::Image::NO_DELETE, 4);
            geom->dirtyDisplayList();
        }

        return ACTION_DONE;
        //return ACTION_CALL_ON_MISS;
    }

    void miss()
    {
        unregister = true;
    }

    float getWidth() const
    { return width; }

    float getHeight() const
    { return height; }

    float getXpos() const
    { return xPos; }

    float getYpos() const
    { return yPos; }

    void setPos(float x, float y, float)
    {
        xPos = x;
        yPos = y;
        dcs->setTranslation(x, y + getHeight(), 0.f);
    }

    vruiTransformNode *getDCS()
    {
        return dcs;
    }

    osg::ref_ptr<osg::Geode> createGeode()
    {
        vertices = new osg::Vec3Array(4);
        (*vertices)[0].set(0.f, 0.f, 1e-1f);
        (*vertices)[1].set(width, 0.f, 1e-1f);
        (*vertices)[2].set(width, height, 1e-1f);
        (*vertices)[3].set(0.f, height, 1e-1f);

        texcoords = new osg::Vec2Array(4);
        (*texcoords)[0].set(0.f, 0.f);
        (*texcoords)[1].set(1.f, 0.f);
        (*texcoords)[2].set(1.f, 1.f);
        (*texcoords)[3].set(0.f, 1.f);

        alphaTexture = new osg::Texture2D;
        alphaTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        alphaTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        alphaTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
        alphaTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);

        updateAlphaTextureData();

        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->setImage(width, height, 1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE,
                        alphaTextureData.data(), osg::Image::NO_DELETE, 4);
        alphaTexture->setImage(image.get());

        stateset = new osg::StateSet;
        stateset->setGlobalDefaults();
        stateset->setTextureAttributeAndModes(0, alphaTexture.get(), osg::StateAttribute::ON);

        geom = new osg::Geometry;
        geom->setVertexArray(vertices.get());
        geom->setTexCoordArray(0,texcoords.get());
        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
    
        geode = new osg::Geode;
        geode->setStateSet(stateset.get());
        geode->addDrawable(geom);

        return geode.get();
    }

    void updateAlphaTextureData()
    {
        alphaTextureData.resize(width * height * 4);
        for (int x=0; x<width; ++x) {
            for (int y=0; y<height; ++y) {
                float valy = y/float(height-1);
                if (valy < opacity.data[x]) {
                    alphaTextureData[(y*(int)width+x)*4] = 255;
                    alphaTextureData[(y*(int)width+x)*4+1] = 255;
                    alphaTextureData[(y*(int)width+x)*4+2] = 255;
                    alphaTextureData[(y*(int)width+x)*4+3] = 255;
                } else {
                    alphaTextureData[(y*(int)width+x)*4] = 0;
                    alphaTextureData[(y*(int)width+x)*4+1] = 0;
                    alphaTextureData[(y*(int)width+x)*4+2] = 0;
                    alphaTextureData[(y*(int)width+x)*4+3] = 0;
                }
            }
        }
    }

    void setOpacityUpdateFunc(coOpacityUpdateFunc func, void *userData)
    {
        opacity.updateFunc = func;
        opacity.userData = userData;
    }
private:
    float width=512.f, height=256.f;
    float xPos=0.f, yPos=0.f;
    vrui::OSGVruiTransformNode *dcs{nullptr};
    vrui::coCombinedButtonInteraction *interactionA{nullptr};
    bool unregister{false}; // mouse left, unregister interaction(s)

    osg::ref_ptr<osg::Geometry> geom;
    osg::ref_ptr<osg::Geode> geode;
    osg::ref_ptr<osg::StateSet> stateset;
    osg::ref_ptr<osg::Vec3Array> vertices;
    osg::ref_ptr<osg::Vec2Array> texcoords;

    osg::ref_ptr<osg::Texture2D> alphaTexture;
    std::vector<unsigned char> alphaTextureData;

    struct {
        std::vector<float> data;
        coOpacityUpdateFunc updateFunc{nullptr};
        void *userData{nullptr};
        bool updated{false};
    } opacity;
};

coTransfuncEditor::coTransfuncEditor()
{
    panel = new coPanel(new coFlatPanelGeometry(coUIElement::BLACK));
    handle = new coPopupHandle("TFE");
    canvas = new Canvas;
    panel->addElement(canvas);
    panel->resize();

    frame = new coFrame("UI/Frame");
    frame->addElement(panel);
    handle->addElement(frame);

    show();
}

coTransfuncEditor::~coTransfuncEditor()
{
    delete handle;
    delete panel;
    delete frame;
}

void coTransfuncEditor::show()
{
    handle->setVisible(true);
    panel->show(canvas);
}

void coTransfuncEditor::hide()
{
    //panel->hide(canvas);
    handle->setVisible(false);
}

void coTransfuncEditor::update()
{
    canvas->update();
    handle->update();
}

void coTransfuncEditor::setOpacityUpdateFunc(coOpacityUpdateFunc func, void *userData)
{
    canvas->setOpacityUpdateFunc(func, userData);
}