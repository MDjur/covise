
#ifndef COVUSE_UTIL_COMORMAP_H
#define COVUSE_UTIL_COMORMAP_H

#include <cover/ui/Button.h>
#include <cover/ui/Group.h>
#include <cover/ui/Menu.h>
#include <cover/ui/SelectionList.h>
#include <cover/ui/Slider.h>

#include <map>
#include <osg/Vec4>
#include <string>
#include <vector>

#include "util/coExport.h"

namespace covise {

struct ColorMap {
  std::vector<float> r, g, b, a, samplingPoints;
  float min = 0.0, max = 0.0;
  int steps = 32;
};

typedef std::map<std::string, ColorMap> ColorMaps;
PLUGIN_UTILEXPORT ColorMaps readColorMaps();
osg::Vec4 PLUGIN_UTILEXPORT getColor(float val, const ColorMap &colorMap,
                                     float min = 0, float max = 1);
// same logic as colors module, but sets linear sampling points
ColorMap PLUGIN_UTILEXPORT interpolateColorMap(const ColorMap &cm, int numSteps);
ColorMap PLUGIN_UTILEXPORT upscale(const ColorMap &baseMap, size_t numSteps);

class PLUGIN_UTILEXPORT ColorMapSelector {
 public:
  ColorMapSelector(opencover::ui::Menu &menu);
  ColorMapSelector(opencover::ui::Group &menu);

  bool setValue(const std::string &colorMapName);
  osg::Vec4 getColor(float val, float min = 0, float max = 1);
  const ColorMap &selectedMap() const;
  void setCallback(const std::function<void(const ColorMap &)> &f);

 private:
  opencover::ui::SelectionList *m_selector;
  const ColorMaps m_colors;
  ColorMaps::const_iterator m_selectedMap;
  void updateSelectedMap();
  void init();
};

class PLUGIN_UTILEXPORT ColorMapMenu {
 public:
  ColorMapMenu(opencover::ui::Group &group);

  void setMinBounds(float min, float max) {
    setSliderBounds(m_minAttribute, min, max);
  }

  void setMaxBounds(float min, float max) {
    setSliderBounds(m_maxAttribute, min, max);
  }

  void setNumStepsBounds(int min, int max) { setSliderBounds(m_numSteps, min, max); }
  void setCallback(const std::function<void(const ColorMap &)> &f);

  auto getColor(float val);
  auto getColorMap() { return m_colorMap; }
  const auto getMin() const { return m_minAttribute->value(); }
  const auto getMax() const { return m_maxAttribute->value(); }
  const auto getNumSteps() const { return m_numSteps->value(); }
  const ColorMapSelector &getSelector() const { return *m_selector; }

 private:
  void init();
  void initUI();
  void initShow();
  void initSteps();
  void initColorMap();
  opencover::ui::Slider *createSlider(
      const std::string &name, const opencover::ui::Slider::ValueType &min,
      const opencover::ui::Slider::ValueType &max,
      const opencover::ui::Slider::Presentation &presentation,
      const opencover::ui::Slider::ValueType &initial,
      std::function<void(float, bool)> callback);
  void setSliderBounds(opencover::ui::Slider *slider, float min, float max) {
    slider->setBounds(min, max);
  }
  void sliderCallback(opencover::ui::Slider *slider, float &toSet, float value,
                      bool moving, bool predicateCheck = false);

  // dont delete these pointers, they are managed by the ui
  opencover::ui::Group *m_colorMapGroup = nullptr;
  opencover::ui::Slider *m_minAttribute = nullptr;
  opencover::ui::Slider *m_maxAttribute = nullptr;
  opencover::ui::Slider *m_numSteps = nullptr;
  opencover::ui::Button *m_show = nullptr;

  std::unique_ptr<ColorMapSelector> m_selector;
  std::shared_ptr<ColorMap> m_colorMap;
};
}  // namespace covise

#endif
