/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// class coColorBar
//
// derived from coMenuItem
// colorbar is a window containing a texture and labels
//
// Initial version: 2002-01, dr
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2002 by Vircinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef _CO_COLOR_BAR_H_
#define _CO_COLOR_BAR_H_

#define MAX_LABELS 18
#include <OpenVRUI/coMenuItem.h>
#include <util/coTypes.h>
#include <vector>
#include "coColorMap.h"
namespace vrui
{
class coLabel;
class coRowContainer;
class coTexturedBackground;
class coColoredBackground;
}

namespace opencover
{


/** class coColorBar, derived from coMenuItem
 *  colorbar is a window containing a texture and labels
 */
class PLUGIN_UTILEXPORT coColorBar : public vrui::coMenuItem
{
private:
    vrui::coColoredBackground *background_ = nullptr;

    // topspacer and texture in vertical row container
    vrui::coColoredBackground *vspace_ = nullptr;
    vrui::coTexturedBackground *texture_ = nullptr;
    vrui::coRowContainer *textureAndVspace_ = nullptr;

    // hspacers and labels in horiz containers, all labels in vert container
    vrui::coLabel *labels_[MAX_LABELS];
    vrui::coLabel *speciesLabel_ = nullptr;
    vrui::coTexturedBackground *hspaces_[MAX_LABELS];
    vrui::coRowContainer *labelAndHspaces_[MAX_LABELS];
    vrui::coColoredBackground *vspaces_[MAX_LABELS];
    vrui::coRowContainer *allLabels_ = nullptr;

    // horiz container around textureAndVspacer and all labels
    vrui::coRowContainer *textureAndLabels_ = nullptr;
    // vert container around textureAndLabels_ and species label
    vrui::coRowContainer *everything_ = nullptr;

    int numLabels_ = 0; // number of labels, max
    float labelValues_[MAX_LABELS]; // numerical values of labels
    char format_str_[32]; // precision of float values

   //  const opencover::ColorMap &map_;
    std::vector<unsigned char> image_, tickImage_;
    std::string name_; // the name of the colors module for example Colors_1

    void makeImage(const ColorMap &map, bool swapped);
    void makeTickImage();
    void makeLabelValues(const ColorMap &map);

public:
    /** constructor
       *  create texture and labels, put them nto containers
       *  @param name the name of the colorbar, identical with module name, eg, g, Colors_1
       *  @param map color map to display
       */
    coColorBar(const std::string &name, const ColorMap &map, bool inMenu=true);

    /// destructor
    ~coColorBar();

      /// display a new color map
      void update(const ColorMap &map);

    /** get name
       *  @return name the name of the colorbar, identical with module name, eg, g, Colors_1
       */
    const char *getName() const override;

    virtual vrui::coUIElement *getUIElement() override;

    /// get the Element's classname
    virtual const char *getClassName() const override;
    /// check if the Element or any ancestor is this classname
    virtual bool isOfClassName(const char *) const override;
};
}

#endif
