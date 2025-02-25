#include "coColorMap.h"

#include <config/CoviseConfig.h>
#include <config/coConfig.h>
#include <cover/ui/Slider.h>

#include <cassert>
#include <iostream>
#include <memory>

using namespace std;
using namespace opencover;

covise::ColorMaps covise::readColorMaps() {
  // read the name of all colormaps in file

  covise::coCoviseConfig::ScopeEntries colorMapEntries =
      coCoviseConfig::getScopeEntries("Colormaps");
  ColorMaps colorMaps;
#ifdef NO_COLORMAP_PARAM
  colorMapEntries["COVISE"];
#else
  // colorMapEntries["Editable"];
#endif

  for (const auto &map : colorMapEntries) {
    string name = "Colormaps." + map.first;

    auto no = coCoviseConfig::getScopeEntries(name).size();
    ColorMap &colorMap = colorMaps.emplace(map.first, ColorMap()).first->second;
    // read all sampling points
    float diff = 1.0f / (no - 1);
    float pos = 0;
    for (int j = 0; j < no; j++) {
      string tmp = name + ".Point:" + std::to_string(j);
      ColorMap cm;
      colorMap.r.push_back(coCoviseConfig::getFloat("r", tmp, 0));
      colorMap.g.push_back(coCoviseConfig::getFloat("g", tmp, 0));
      colorMap.b.push_back(coCoviseConfig::getFloat("b", tmp, 0));
      colorMap.a.push_back(coCoviseConfig::getFloat("a", tmp, 1));
      colorMap.samplingPoints.push_back(coCoviseConfig::getFloat("x", tmp, pos));
      pos += diff;
    }
  }
  return colorMaps;
}

osg::Vec4 covise::getColor(float val, const covise::ColorMap &colorMap, float min,
                           float max) {
  assert(val >= min && val <= max);
  val = 1 / (max - min) * (val - min);

  size_t idx = 0;
  for (; idx < colorMap.samplingPoints.size() &&
         colorMap.samplingPoints[idx + 1] < val;
       idx++) {
  }

  double d = (val - colorMap.samplingPoints[idx]) /
             (colorMap.samplingPoints[idx + 1] - colorMap.samplingPoints[idx]);
  osg::Vec4 color;
  color[0] = ((1 - d) * colorMap.r[idx] + d * colorMap.r[idx + 1]);
  color[1] = ((1 - d) * colorMap.g[idx] + d * colorMap.g[idx + 1]);
  color[2] = ((1 - d) * colorMap.b[idx] + d * colorMap.b[idx + 1]);
  color[3] = ((1 - d) * colorMap.a[idx] + d * colorMap.a[idx + 1]);

  return color;
}

covise::ColorMap covise::interpolateColorMap(const covise::ColorMap &cm,
                                             int numSteps) {
  covise::ColorMap interpolatedMap;
  interpolatedMap.r.resize(numSteps);
  interpolatedMap.g.resize(numSteps);
  interpolatedMap.b.resize(numSteps);
  interpolatedMap.a.resize(numSteps);
  interpolatedMap.samplingPoints.resize(numSteps);
  auto numColors = cm.samplingPoints.size();
  double delta = 1.0 / (numSteps - 1) * (numColors - 1);
  double x;
  int i;

  delta = 1.0 / (numSteps - 1);
  int idx = 0;
  for (i = 0; i < numSteps - 1; i++) {
    x = i * delta;
    while (cm.samplingPoints[(idx + 1)] <= x) {
      idx++;
      if (idx > numColors - 2) {
        idx = numColors - 2;
        break;
      }
    }

    double d = (x - cm.samplingPoints[idx]) /
               (cm.samplingPoints[idx + 1] - cm.samplingPoints[idx]);
    interpolatedMap.r[i] = (float)((1 - d) * cm.r[idx] + d * cm.r[idx + 1]);
    interpolatedMap.g[i] = (float)((1 - d) * cm.g[idx] + d * cm.g[idx + 1]);
    interpolatedMap.b[i] = (float)((1 - d) * cm.b[idx] + d * cm.b[idx + 1]);
    interpolatedMap.a[i] = (float)((1 - d) * cm.a[idx] + d * cm.a[idx + 1]);
    interpolatedMap.samplingPoints[i] = (float)i / (numSteps - 1);
  }
  interpolatedMap.r[numSteps - 1] = cm.r[(numColors - 1)];
  interpolatedMap.g[numSteps - 1] = cm.g[(numColors - 1)];
  interpolatedMap.b[numSteps - 1] = cm.b[(numColors - 1)];
  interpolatedMap.a[numSteps - 1] = cm.a[(numColors - 1)];
  interpolatedMap.samplingPoints[numSteps - 1] = 1;

  interpolatedMap.min = cm.min;
  interpolatedMap.max = cm.max;
  interpolatedMap.steps = numSteps;

  return interpolatedMap;
}

covise::ColorMapSelector::ColorMapSelector(opencover::ui::Group &group)
    : m_selector(new opencover::ui::SelectionList(&group, "mapChoice")),
      m_colors(readColorMaps()) {
  init();
}

covise::ColorMapSelector::ColorMapSelector(opencover::ui::Menu &menu)
    : m_selector(new opencover::ui::SelectionList{&menu, "mapChoice"}),
      m_colors(readColorMaps()) {
  init();
}

bool covise::ColorMapSelector::setValue(const std::string &colorMapName) {
  auto it = m_colors.find(colorMapName);
  if (it == m_colors.end()) return false;

  m_selector->select(std::distance(m_colors.begin(), it));
  updateSelectedMap();
  return true;
}

osg::Vec4 covise::ColorMapSelector::getColor(float val, float min, float max) {
  return covise::getColor(val, m_selectedMap->second, min, max);
}

const covise::ColorMap &covise::ColorMapSelector::selectedMap() const {
  return m_selectedMap->second;
}

void covise::ColorMapSelector::setCallback(
    const std::function<void(const ColorMap &)> &f) {
  m_selector->setCallback([this, f](int index) {
    updateSelectedMap();
    f(selectedMap());
  });
}

void covise::ColorMapSelector::updateSelectedMap() {
  m_selectedMap = m_colors.begin();
  std::advance(m_selectedMap, m_selector->selectedIndex());
  assert(m_selectedMap != m_colors.end());
}

void covise::ColorMapSelector::init() {
  for (auto &n : m_colors) m_selector->append(n.first);
  m_selector->select(0);
  m_selectedMap = m_colors.begin();

  m_selector->setCallback([this](int index) { updateSelectedMap(); });
}

covise::ColorMapMenu::ColorMapMenu(opencover::ui::Group &group)
    : m_colorMapGroup(new opencover::ui::Group(&group, "ColorMap")),
      m_selector(std::make_unique<ColorMapSelector>(*m_colorMapGroup)) {
  init();
}

void covise::ColorMapMenu::sliderCallback(opencover::ui::Slider *slider,
                                          float &toSet, float value, bool moving,
                                          bool predicateCheck) {
  if (!moving) return;
  if (predicateCheck) {
    slider->setValue(toSet);
    return;
  }
  toSet = value;
}

opencover::ui::Slider *covise::ColorMapMenu::createSlider(
    const std::string &name, const ui::Slider::ValueType &min,
    const ui::Slider::ValueType &max, const ui::Slider::Presentation &presentation,
    const ui::Slider::ValueType &initial,
    std::function<void(float, bool)> callback) {
  auto slider = new ui::Slider(m_colorMapGroup, name);
  slider->setBounds(min, max);
  slider->setPresentation(presentation);
  slider->setValue(initial);
  slider->setCallback(callback);
  return slider;
}

void covise::ColorMapMenu::initSteps() {
  m_numSteps = createSlider(
      "steps", 1, 1024, ui::Slider::AsDial, 1, [this](float value, bool moving) {
        if (!moving) return;
        *m_colorMap = covise::interpolateColorMap(*m_colorMap, value);
      });
  m_numSteps->setScale(ui::Slider::Linear);
  m_numSteps->setIntegral(true);
  m_numSteps->setLinValue(32);
}

void covise::ColorMapMenu::initColorMap() {
  m_colorMap = std::make_shared<ColorMap>(m_selector->selectedMap());
}

void covise::ColorMapMenu::initShow() {
  m_show = new ui::Button(m_colorMapGroup, "Show");
  m_show->setCallback([this](bool) {
    std::cout << "ColorMapMenu::show not implemented" << std::endl;
  });
}

void covise::ColorMapMenu::initUI() {
  initShow();
  initColorMap();
  m_minAttribute = createSlider(
      "min", 0, 1, ui::Slider::AsDial, 0, [this](float value, bool moving) {
        sliderCallback(m_minAttribute, m_colorMap->min, value, moving,
                       value > m_maxAttribute->value());
      });
  m_maxAttribute = createSlider(
      "max", 0, 1, ui::Slider::AsDial, 1, [this](float value, bool moving) {
        sliderCallback(m_maxAttribute, m_colorMap->max, value, moving,
                       value < m_minAttribute->value());
      });
  initSteps();
}

void covise::ColorMapMenu::init() { initUI(); }

void covise::ColorMapMenu::setCallback(
    const std::function<void(const ColorMap &)> &f) {
  m_selector->setCallback([this, f](const ColorMap &cm) {
    *m_colorMap = interpolateColorMap(cm, m_numSteps->value());
    f(*m_colorMap);
  });
}

auto covise::ColorMapMenu::getColor(float val) {
  return covise::getColor(val, *m_colorMap, m_colorMap->min, m_colorMap->max);
}
