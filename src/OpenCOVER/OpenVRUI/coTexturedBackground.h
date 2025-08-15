/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

//-*-c++-*-
#ifndef CO_TEXTURED_BACKGROUND_H
#define CO_TEXTURED_BACKGROUND_H

#include <OpenVRUI/coAction.h>
#include <OpenVRUI/coBackground.h>

#ifndef __linux__
typedef unsigned char uchar; ///< abbreviation for unsigned char
typedef unsigned short ushort; ///< abbreviation for unsigned short
typedef unsigned int uint; ///< abbreviation for unsigned int
typedef unsigned long ulong; ///< abbreviation for unsigned long
typedef signed char schar; ///< abbreviation for signed char
typedef signed short sshort; ///< abbreviation for signed short
typedef signed int sint; ///< abbreviation for signed int
typedef signed long slong; ///< abbreviation for signed long
#endif
#undef ACTION_BUTTON
#undef DRIVE_BUTTON
#undef XFORM_BUTTON

#include <string>

namespace vrui
{

class coTexturedBackground;
class vruiHit;

/** Overwrite the routines of this class to get callback functions for
  when the pointer intersected the texture and the button was pressed.
*/
class OPENVRUIEXPORT coTexturedBackgroundActor
{
public:
    virtual ~coTexturedBackgroundActor()
    {
    }
    virtual void texturePointerClicked(coTexturedBackground *, float, float);
    virtual void texturePointerReleased(coTexturedBackground *, float, float);
    virtual void texturePointerDragged(coTexturedBackground *, float, float);
    virtual void texturePointerMoved(coTexturedBackground *, float, float);
    virtual void texturePointerLeft(coTexturedBackground *);
};

/** This class provides background for GUI elements.
    The texture of this background changes according to the elements state
    (normal/highlighted/disabled)
    A background should contain only one child, use another container to layout
    multiple chlidren inside the frame.
*/
class OPENVRUIEXPORT coTexturedBackground
    : public coBackground,
      public coAction
{
public:

    class TextureSet
    {

    public:
        enum TexturePixelFormatType { PF_DEFAULT = 0, PF_RGB = 0x1907, PF_RGBA = 0x1908, PF_GL_LUMINANCE = 0x1909, PF_BGRA = 0x80E1 };
        TextureSet(const uint* nt, const uint* ht, const uint* dt, int comp, int s, int t, int r, TextureSet::TexturePixelFormatType pf = TextureSet::PF_DEFAULT)
            : start(0.0f, 0.0f)
            , end(1.0f, 1.0f)
        {
            normalTextureImage = nt;
            highlightedTextureImage = ht;
            disabledTextureImage = dt;
            this->comp = comp;
            this->s = s;
            this->t = t;
            this->r = r;
            pixelFormat = pf;
        }

        const uint* normalTextureImage;
        const uint* highlightedTextureImage;
        const uint* disabledTextureImage;
        int comp;
        int s;
        int t;
        int r;
        TexturePixelFormatType pixelFormat = PF_DEFAULT;

        struct TexCoord
        {
            TexCoord(float xv, float yv)
                : x(xv)
                , y(yv)
            {
            }
            float x;
            float y;
        };

        TexCoord start;
        TexCoord end;
    };

    coTexturedBackground(const std::string &normalTexture, const std::string &highlightTexture,
                         const std::string &disabledTexture, coTexturedBackgroundActor *actor = 0);
    coTexturedBackground(const uint *normalImage, const uint *highlightImage,
                         const uint *disabledImage, int comp, int ns, int nt, int nr,
                         coTexturedBackgroundActor *actor = 0, TextureSet::TexturePixelFormatType pixelFormat = TextureSet::PF_DEFAULT);
    virtual ~coTexturedBackground();


    virtual int hit(vruiHit *hit);
    virtual void miss();

    virtual void setEnabled(bool en);
    virtual void setHighlighted(bool hl);
    /// functions activates or deactivates the item
    virtual void setActive(bool a);

    virtual void shrinkToMin();

    //void createGeode();
    void setRepeat(bool repeat);
    bool getRepeat() const;

    void setUpdated(bool update)
    {
        updated = update;
    }
    bool getUpdated()
    {
        return updated;
    }

    void setTexSize(float, float); // size of the texture, ==0 is fit to Background

    float getTexXSize() const
    {
        return texXSize;
    }
    float getTexYSize() const
    {
        return texYSize;
    }

    void setImage(const uint *normalImage, const uint *highlightImage,
                  const uint *disabledImage, int comp, int ns, int nt, int nr, TextureSet::TexturePixelFormatType pixelFormat=TextureSet::PF_DEFAULT);

    /// get the Element's classname
    virtual const char *getClassName() const;
    /// check if the Element or any ancestor is this classname
    virtual bool isOfClassName(const char *) const;

    inline TextureSet *getCurrentTextures()
    {
        return currentTextures;
    }

    inline const std::string &getNormalTexName() const
    {
        return normalTexName;
    }
    inline const std::string &getHighlightTexName() const
    {
        return highlightTexName;
    }
    inline const std::string &getDisabledTexName() const
    {
        return disabledTexName;
    }

    void setScale(float s);
    float getScale();

protected:
    coTexturedBackgroundActor *myActor; ///< action listener, triggered on pointer intersections

private:
    std::string normalTexName; ///< Name of the normal texture
    std::string highlightTexName; ///< Name of the highlighted texture
    std::string disabledTexName; ///< Name of the disabled texture

    TextureSet *currentTextures;

    float texXSize;
    float texYSize;

    bool repeat;
    bool active_;
    float myScale;

    // Pinkowski, 20.09.2007
    bool updated;
};
}
#endif
