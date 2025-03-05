/****************************************************************************\
 **                                                          (C)2024 HLRS  **
 **                                                                        **
 ** Description: OpenCOVER Plug-In for reading building energy data        **
 **                                                                        **
 **                                                                        **
 ** Author: Leyla Kern, Marko Djuric                                       **
 **                                                                        **
 ** TODO:                                                                  **
 **  [ ] fetch lat lon from googlemaps                                     **
 **  [x] make REST lib independent from ennovatis general use              **
 **  [x] update via REST in background                                     **
 **                                                                        **
 ** History:                                                               **
 **  2024  v1                                                              **
 **  Marko Djuric 02.2024: add ennovatis client                            **
 **  Marko Djuric 10.2024: add citygml interface                           **
 **                                                                        **
 **                                                                        **
\****************************************************************************/

#include "Energy.h"
#include "ui/historic/Device.h"
#include "ui/ennovatis/EnnovatisDevice.h"
#include "ui/ennovatis/EnnovatisDeviceSensor.h"
#include <build_options.h>
#include <config/CoviseConfig.h>

// COVER
#include <PluginUtil/coColorMap.h>
#include <PluginUtil/coShaderUtil.h>
#include <cover/coVRAnimationManager.h>
#include <cover/coVRFileManager.h>
#include <cover/coVRTui.h>
#include <cover/ui/ButtonGroup.h>
#include <cover/ui/EditField.h>
#include <cover/ui/Group.h>
#include <cover/ui/SelectionList.h>
#include <cover/ui/Slider.h>
#include <cover/ui/View.h>
#include <utils/read/csv/csv.h>
#include <utils/string/LevenshteinDistane.h>

// Ennovatis
#include <lib/ennovatis/building.h>
#include <lib/ennovatis/csv.h>
#include <lib/ennovatis/date.h>
#include <lib/ennovatis/rest.h>
#include <lib/ennovatis/sax.h>

// std
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <regex>

// OSG
#include <osg/Geometry>
#include <osg/Group>
#include <osg/LineWidth>
#include <osg/MatrixTransform>
#include <osg/Node>
#include <osg/Sequence>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Switch>
#include <osg/Vec3>
#include <osg/Version>
#include <osg/ref_ptr>
#include <osgUtil/Optimizer>

// boost
#include <boost/filesystem.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/tokenizer.hpp>

#include "OpenConfig/covconfig/array.h"

// presentation
#include <app/presentation/CityGMLBuilding.h>
#include <app/presentation/EnergyGrid.h>
#include <app/presentation/PrototypeBuilding.h>
#include <app/presentation/TxtInfoboard.h>

// core
#include <lib/core/simulation/grid.h>
#include <lib/core/utils/color.h>
#include <lib/core/simulation/heating.h>
#include <lib/core/utils/osgUtils.h>

using namespace opencover;
using namespace opencover::utils::read;
using namespace opencover::utils::string;
using namespace energy;

namespace fs = boost::filesystem;

namespace {

constexpr bool debug = build_options.debug_ennovatis;
// regex for dd.mm.yyyy
const std::regex dateRgx(
    R"(((0[1-9])|([12][0-9])|(3[01]))\.((0[0-9])|(1[012]))\.((20[012]\d|19\d\d)|(1\d|2[0123])))");
ennovatis::rest_request_handler m_debug_worker;

// Compare two string numbers as integer using std::stoi
bool helper_cmpStrNo_as_int(const std::string &strtNo, const std::string &strtNo2) {
  try {
    int intStrtNo = std::stoi(strtNo), intStrtNo2 = std::stoi(strtNo2);
    auto validConversion =
        strtNo == std::to_string(intStrtNo) && strtNo2 == std::to_string(intStrtNo2);
    if (intStrtNo2 == intStrtNo && validConversion) return true;
  } catch (...) {
  }
  return false;
}

/**
 * @brief Compares two string street numbers in the format ("<streetname>
 * <streetnumber>").
 *
 * The function compares the street numbers of the two street names as string
 * and integer. If the street numbers are equal, the function returns true.
 * @param strtName The first string street name.
 * @param strtName2 The second string street name.
 * @return true if the street numbers are equal, otherwise false.
 */
bool cmpStrtNo(const std::string &strtName, const std::string &strtName2) {
  auto strtNo = strtName.substr(strtName.find_last_of(" ") + 1);
  auto strtNo2 = strtName2.substr(strtName2.find_last_of(" ") + 1);

  // compare in lower case str
  auto lower = [](unsigned char c) { return std::tolower(c); };
  std::transform(strtNo2.begin(), strtNo2.end(), strtNo2.begin(), lower);
  std::transform(strtNo.begin(), strtNo.end(), strtNo.begin(), lower);
  if (strtNo2 == strtNo) return true;

  // compare as integers
  return helper_cmpStrNo_as_int(strtNo, strtNo2);
};

float computeDistributionCenter(const std::vector<float> &values) {
  float sum = 0;
  for (auto &value : values) sum += value;
  return sum / values.size();
}

}  // namespace

/* #region GENERAL */
EnergyPlugin *EnergyPlugin::m_plugin = nullptr;

EnergyPlugin::EnergyPlugin()
    : coVRPlugin(COVER_PLUGIN_NAME),
      ui::Owner("EnergyPlugin", cover->ui),
      m_offset(3),
      m_req(nullptr),
      m_ennovatis(new osg::Group()),
      m_switch(new osg::Switch()),
      m_grid(new osg::Switch()),
      m_sequenceList(new osg::Sequence()),
      m_Energy(new osg::MatrixTransform()),
      m_cityGML(new osg::Group()),
      m_heatingGroup(new osg::Group()),
      m_powerGroup(new osg::Group()) {
  // need to save the config on exit => will only be saved when COVER is closed
  // correctly via q or closing the window
  config()->setSaveOnExit(true);
  fprintf(stderr, "Starting Energy Plugin\n");
  m_plugin = this;

  m_buildings = std::make_unique<ennovatis::Buildings>();

  m_sequenceList->setName("DB");
  m_ennovatis->setName("Ennovatis");
  m_cityGML->setName("CityGML");

  m_Energy->setName("Energy");
  cover->getObjectsRoot()->addChild(m_Energy);

  m_switch->setName("Switch");
  m_switch->addChild(m_sequenceList);
  m_switch->addChild(m_ennovatis);
  m_switch->addChild(m_cityGML);

  m_grid->setName("EnergyGrids");

  m_Energy->addChild(m_switch);
  m_Energy->addChild(m_grid);

  GDALAllRegister();

  m_SDlist.clear();

  EnergyTab = new ui::Menu("Energy_Campus", EnergyPlugin::m_plugin);
  EnergyTab->setText("Energy Campus");

  // db
  componentGroup = new ui::ButtonGroup(EnergyTab, "ComponentGroup");
  componentGroup->setDefaultValue(Strom);
  componentList = new ui::Menu(EnergyTab, "Component");
  componentList->setText("Messwerte (jährlich)");
  StromBt = new ui::Button(componentList, "Strom", componentGroup, Strom);
  WaermeBt = new ui::Button(componentList, "Waerme", componentGroup, Waerme);
  KaelteBt = new ui::Button(componentList, "Kaelte", componentGroup, Kaelte);
  componentGroup->setCallback(
      [this](int value) { setComponent(Components(value)); });

  initEnnovatisUI();
  initCityGMLUI();
  initSimUI();

  m_offset =
      configFloatArray("General", "offset", std::vector<double>{0, 0, 0})->value();
}

EnergyPlugin::~EnergyPlugin() {
  auto root = cover->getObjectsRoot();

  if (m_cityGML) {
    restoreCityGMLDefaultStatesets();
    for (unsigned int i = 0; i < m_cityGML->getNumChildren(); ++i) {
      auto child = m_cityGML->getChild(i);
      root->addChild(child);
    }
    core::utils::osgUtils::deleteChildrenFromOtherGroup(m_cityGML, root);
  }

  if (m_Energy) {
    root->removeChild(m_Energy.get());
  }

  config()->save();
  m_plugin = nullptr;
}

std::pair<PJ *, PJ_COORD> EnergyPlugin::initProj() {
  ProjTrans pjTrans;
  pjTrans.projFrom = configString("General", "projFrom", "default")->value();
  pjTrans.projTo = configString("General", "projTo", "default")->value();
  auto P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, pjTrans.projFrom.c_str(),
                                  pjTrans.projTo.c_str(), NULL);
  PJ_COORD coord;
  coord.lpzt.z = 0.0;
  coord.lpzt.t = HUGE_VAL;
  bool mapdrape = true;

  if (!P) {
    fprintf(stderr,
            "Energy Plugin: Ignore mapping. No valid projection was "
            "found between given proj string in "
            "config EnergyCampus.toml\n");
    mapdrape = false;
  }
  return std::make_pair(P, coord);
}

void EnergyPlugin::projTransLatLon(float &lat, float &lon) {
  auto [P, coord] = initProj();
  coord.lpzt.lam = lon;
  coord.lpzt.phi = lat;

  coord = proj_trans(P, PJ_FWD, coord);

  lon = coord.xy.x + m_offset[0];
  lat = coord.xy.y + m_offset[1];
}

bool EnergyPlugin::update() {
  for (auto s = m_SDlist.begin(); s != m_SDlist.end(); s++) {
    if (s->second.empty()) continue;
    for (auto timeElem : s->second) {
      if (timeElem) timeElem->update();
    }
  }

  if constexpr (debug) {
    auto result = m_debug_worker.getResult();
    if (result)
      for (auto &requ : *result) std::cout << "Response:\n" << requ << "\n";
  }

  for (auto &sensor : m_ennovatisDevicesSensors) sensor->update();

  for (auto &[name, sensor] : m_cityGMLObjs) sensor->update();

  return false;
}

void EnergyPlugin::setTimestep(int t) {
  m_sequenceList->setValue(t);
  for (auto &sensor : m_ennovatisDevicesSensors) sensor->setTimestep(t);

  for (auto &[_, sensor] : m_cityGMLObjs) sensor->updateTime(t);
  m_heatingSimUI->updateTime(t);
}

void EnergyPlugin::switchTo(const osg::ref_ptr<osg::Node> child,
                            osg::ref_ptr<osg::Switch> parent) {
  parent->setAllChildrenOff();
  parent->setChildValue(child, true);
}

bool EnergyPlugin::init() {
  auto dbPath = configString("CSV", "filename", "default")->value();
  auto channelIdJSONPath = configString("Ennovatis", "jsonPath", "default")->value();
  // csv contains only updated buildings
  auto channelIdCSVPath = configString("Ennovatis", "csvPath", "default")->value();
  ProjTrans pjTrans;
  pjTrans.projFrom = configString("General", "projFrom", "default")->value();
  pjTrans.projTo = configString("General", "projTo", "default")->value();

  initRESTRequest();

  if constexpr (debug) {
    std::cout << "Load database: " << dbPath << std::endl;
    std::cout << "Load channelIDs: " << channelIdJSONPath << std::endl;
  }

  if (loadDB(dbPath, pjTrans))
    std::cout << "Database loaded in cache" << std::endl;
  else
    std::cout << "Database not loaded" << std::endl;

  if (loadChannelIDs(channelIdJSONPath, channelIdCSVPath))
    std::cout << "Ennovatis channelIDs loaded in cache" << std::endl;
  else
    std::cout << "Ennovatis channelIDs not loaded" << std::endl;

  auto noMatches = updateEnnovatisBuildings(m_SDlist);

  if constexpr (debug) {
    int i = 0;
    std::cout << "Matches between devices and buildings:" << std::endl;
    for (auto &[device, building] : m_devBuildMap)
      std::cout << ++i << ": Device: " << device->getInfo()->strasse
                << " -> Building: " << building->getName() << std::endl;

    std::cout << "No matches for the following buildings:" << std::endl;
    for (auto &building : *noMatches) std::cout << building->getName() << std::endl;
  }
  initEnnovatisDevices();
  switchTo(m_sequenceList, m_switch);
  initGrid();
  return true;
}

EnergyPlugin::CSVStreamMapPtr EnergyPlugin::getCSVStreams(
    const boost::filesystem::path &dirPath) {
  auto csv_files = std::make_unique<CSVStreamMap>();
  for (auto &entry : fs::directory_iterator(dirPath)) {
    if (fs::is_regular_file(entry)) {
      if (entry.path().extension() == ".csv") {
        auto path = entry.path();
        csv_files->insert(
            {path.stem().string(), std::make_unique<CSVStream>(path.string())});
      }
    }
  }
  return csv_files;
}

void EnergyPlugin::setAnimationTimesteps(size_t maxTimesteps, const void *who) {
  if (maxTimesteps > opencover::coVRAnimationManager::instance()->getNumTimesteps())
    opencover::coVRAnimationManager::instance()->setNumTimesteps(maxTimesteps, who);
}
/* #endregion */

/* #region CITYGML */
void EnergyPlugin::initCityGMLUI() {
  m_cityGMLMenu = new ui::Menu(EnergyTab, "CityGML");
  m_cityGMLEnable = new ui::Button(m_cityGMLMenu, "Enable");
  m_cityGMLEnable->setCallback([this](bool on) { enableCityGML(on); });
}

void EnergyPlugin::enableCityGML(bool on) {
  if (on) {
    if (m_cityGMLObjs.empty()) {
      auto root = cover->getObjectsRoot();
      for (unsigned int i = 0; i < root->getNumChildren(); ++i) {
        osg::ref_ptr<osg::MatrixTransform> child =
            dynamic_cast<osg::MatrixTransform *>(root->getChild(i));
        if (child) {
          auto name = child->getName();
          if (name.find(".gml") != std::string::npos) {
            addCityGMLObjects(child);
            m_cityGML->addChild(child);
          }
        }
      }
      core::utils::osgUtils::deleteChildrenFromOtherGroup(root, m_cityGML);
    }
    switchTo(m_cityGML, m_switch);

    auto influxStatic =
        configString("Simulation", "staticInfluxCSV", "default")->value();
    applyStaticInfluxToCityGML(influxStatic);
  } else {
    switchTo(m_sequenceList, m_switch);
  }
}

void EnergyPlugin::addCityGMLObject(const std::string &name,
                                    osg::ref_ptr<osg::Group> citygmlObjGroup) {
  if (!citygmlObjGroup->getNumChildren()) return;

  if (m_cityGMLObjs.find(name) != m_cityGMLObjs.end()) return;

  auto geodes = core::utils::osgUtils::getGeodes(citygmlObjGroup);
  if (geodes->empty()) return;

  // store default stateset
  saveCityGMLObjectDefaultStateSet(name, *geodes);

  auto boundingbox = core::utils::osgUtils::getBoundingBox(*geodes);
  auto infoboardPos = boundingbox.center();
  infoboardPos.z() +=
      (boundingbox.zMax() - boundingbox.zMin()) / 2 + boundingbox.zMin();
  auto infoboard = std::make_unique<TxtInfoboard>(
      infoboardPos, name, "DroidSans-Bold.ttf", 50, 50, 2.0f, 0.1, 2);
  auto building = std::make_unique<CityGMLBuilding>(*geodes);
  auto sensor = std::make_unique<CityGMLDeviceSensor>(
      citygmlObjGroup, std::move(infoboard), std::move(building),
      m_colorMapMenu->getColorMap());
  m_cityGMLObjs.insert({name, std::move(sensor)});
}

void EnergyPlugin::addCityGMLObjects(osg::ref_ptr<osg::Group> citygmlGroup) {
  using namespace core::utils;
  for (unsigned int i = 0; i < citygmlGroup->getNumChildren(); ++i) {
    osg::ref_ptr<osg::Group> child =
        dynamic_cast<osg::Group *>(citygmlGroup->getChild(i));
    if (child) {
      const auto &name = child->getName();

      // handle quad tree optimized scenegraph
      if (name == "GROUP" || name == "") {
        addCityGMLObjects(child);
        continue;
      }

      addCityGMLObject(name, child);
    }
  }
}

void EnergyPlugin::saveCityGMLObjectDefaultStateSet(const std::string &name,
                                                    const Geodes &citygmlGeodes) {
  Geodes geodesCopy(citygmlGeodes.size());
  for (auto i = 0; i < citygmlGeodes.size(); ++i) {
    auto geode = citygmlGeodes[i];
    geodesCopy[i] =
        dynamic_cast<osg::Geode *>(geode->clone(osg::CopyOp::DEEP_COPY_STATESETS));
  }
  m_cityGMLDefaultStatesets.insert({name, std::move(geodesCopy)});
}

void EnergyPlugin::restoreGeodesStatesets(CityGMLDeviceSensor &sensor,
                                          const std::string &name,
                                          const Geodes &citygmlGeodes) {
  if (m_cityGMLDefaultStatesets.find(name) == m_cityGMLDefaultStatesets.end())
    return;

  if (citygmlGeodes.empty()) return;

  for (auto i = 0; i < citygmlGeodes.size(); ++i) {
    auto gmlDefault = citygmlGeodes[i];
    osg::ref_ptr<osg::Geode> toRestore = sensor.getDrawable(i)->asGeode();
    if (toRestore) {
      toRestore->setStateSet(gmlDefault->getStateSet());
    }
  }
}

void EnergyPlugin::restoreCityGMLDefaultStatesets() {
  for (auto &[name, sensor] : m_cityGMLObjs) {
    osg::ref_ptr<osg::Group> sensorParent = sensor->getParent();
    if (!sensorParent) continue;

    restoreGeodesStatesets(*sensor, name, m_cityGMLDefaultStatesets[name]);
  }
  m_cityGMLDefaultStatesets.clear();
}
/* #endregion */

/* #region ENNOVATIS */
void EnergyPlugin::initEnnovatisUI() {
  m_ennovatisMenu = new ui::Menu(EnergyTab, "Ennovatis");
  m_ennovatisMenu->setText("Ennovatis");

  m_ennovatisSelectionsList =
      new ui::SelectionList(m_ennovatisMenu, "Ennovatis_ChannelType");
  m_ennovatisSelectionsList->setText("Channel Type: ");
  std::vector<std::string> ennovatisSelections;
  for (int i = 0; i < static_cast<int>(ennovatis::ChannelGroup::None); ++i)
    ennovatisSelections.push_back(
        ennovatis::ChannelGroupToString(static_cast<ennovatis::ChannelGroup>(i)));

  m_ennovatisSelectionsList->setList(ennovatisSelections);
  m_enabledEnnovatisDevices =
      new opencover::ui::SelectionList(m_ennovatisMenu, "Enabled_Devices");
  m_enabledEnnovatisDevices->setText("Enabled Devices: ");
  m_enabledEnnovatisDevices->setCallback(
      [this](int value) { selectEnabledDevice(); });
  m_ennovatisChannelList =
      new opencover::ui::SelectionList(m_ennovatisMenu, "Channels");
  m_ennovatisChannelList->setText("Channels: ");

  // TODO: add calender widget instead of txtfields
  m_ennovatisFrom = new ui::EditField(m_ennovatisMenu, "from");
  m_ennovatisTo = new ui::EditField(m_ennovatisMenu, "to");

  m_ennovatisUpdate = new ui::Button(m_ennovatisMenu, "Update");
  m_ennovatisUpdate->setCallback([this](bool on) { updateEnnovatis(); });

  m_ennovatisSelectionsList->setCallback(
      [this](int value) { setEnnovatisChannelGrp(ennovatis::ChannelGroup(value)); });
  m_ennovatisFrom->setCallback(
      [this](const std::string &toSet) { setRESTDate(toSet, true); });
  m_ennovatisTo->setCallback(
      [this](const std::string &toSet) { setRESTDate(toSet, false); });
}

void EnergyPlugin::selectEnabledDevice() {
  auto selected = m_enabledEnnovatisDevices->selectedItem();
  for (auto &sensor : m_ennovatisDevicesSensors) {
    auto building = sensor->getDevice()->getBuildingInfo().building;
    if (building->getName() == selected) {
      sensor->disactivate();
      sensor->activate();
      return;
    }
  }
}

void EnergyPlugin::updateEnnovatis() { updateEnnovatisChannelGrp(); }

void EnergyPlugin::setRESTDate(const std::string &toSet, bool isFrom = false) {
  std::string fromOrTo = (isFrom) ? "From: " : "To: ";
  fromOrTo += toSet;
  if (!std::regex_match(toSet, dateRgx)) {
    std::cout << "Invalid date format for " << fromOrTo
              << " Please use the following format: " << ennovatis::date::dateformat
              << std::endl;
    return;
  }

  auto time = ennovatis::date::str_to_time_point(toSet, ennovatis::date::dateformat);
  bool validTime = (isFrom) ? (time <= m_req->dtt) : (time >= m_req->dtf);
  if (!validTime) {
    std::cout << "Invalid date. (To >= From)" << std::endl;
    if (isFrom)
      m_ennovatisFrom->setValue(ennovatis::date::time_point_to_str(
          m_req->dtf, ennovatis::date::dateformat));
    else
      m_ennovatisTo->setValue(ennovatis::date::time_point_to_str(
          m_req->dtt, ennovatis::date::dateformat));
    return;
  }

  if (isFrom)
    m_req->dtf = time;
  else
    m_req->dtt = time;
}

CylinderAttributes EnergyPlugin::getCylinderAttributes() {
  auto configDefaultColorVec = configFloatArray("Ennovatis", "defaultColorCylinder",
                                                std::vector<double>{0, 0, 0, 1.f})
                                   ->value();
  auto configMaxColorVec = configFloatArray("Ennovatis", "maxColorCylinder",
                                            std::vector<double>{0.0, 0.1, 0.0, 1.f})
                               ->value();
  auto configMinColorVec = configFloatArray("Ennovatis", "minColorCylinder",
                                            std::vector<double>{0.0, 1.0, 0.0, 1.f})
                               ->value();
  auto configDefaultHeightCycl =
      configFloat("Ennovatis", "defaultHeightCylinder", 100.0)->value();
  auto configRadiusCycl = configFloat("Ennovatis", "radiusCylinder", 3.0)->value();
  auto defaultColor = osg::Vec4(configDefaultColorVec[0], configDefaultColorVec[1],
                                configDefaultColorVec[2], configDefaultColorVec[3]);
  auto maxColor = osg::Vec4(configMaxColorVec[0], configMaxColorVec[1],
                            configMaxColorVec[2], configMaxColorVec[3]);
  auto minColor = osg::Vec4(configMinColorVec[0], configMinColorVec[1],
                            configMinColorVec[2], configMinColorVec[3]);
  return CylinderAttributes(configRadiusCycl, configDefaultHeightCycl, maxColor,
                            minColor, defaultColor);
}

void EnergyPlugin::initEnnovatisDevices() {
  m_ennovatis->removeChildren(0, m_ennovatis->getNumChildren());
  m_ennovatisDevicesSensors.clear();
  auto cylinderAttributes = getCylinderAttributes();
  for (auto &b : *m_buildings) {
    cylinderAttributes.position = osg::Vec3(b.getX(), b.getY(), b.getHeight());
    auto drawableBuilding = std::make_unique<PrototypeBuilding>(cylinderAttributes);
    auto infoboardPos = osg::Vec3(b.getX() + cylinderAttributes.radius + 5,
                                  b.getY() + cylinderAttributes.radius + 5,
                                  b.getHeight() + cylinderAttributes.height);
    auto infoboard = std::make_unique<TxtInfoboard>(
        infoboardPos, b.getName(), "DroidSans-Bold.ttf",
        cylinderAttributes.radius * 20, cylinderAttributes.radius * 21, 2.0f, 0.1,
        2);
    auto enDev = std::make_unique<EnnovatisDevice>(
        b, m_ennovatisChannelList, m_req, m_channelGrp, std::move(infoboard),
        std::move(drawableBuilding));
    m_ennovatis->addChild(enDev->getDeviceGroup());
    m_ennovatisDevicesSensors.push_back(std::make_unique<EnnovatisDeviceSensor>(
        std::move(enDev), enDev->getDeviceGroup(), m_enabledEnnovatisDevices));
  }
}

void EnergyPlugin::updateEnnovatisChannelGrp() {
  for (auto &sensor : m_ennovatisDevicesSensors)
    sensor->getDevice()->setChannelGroup(m_channelGrp);
}

void EnergyPlugin::setEnnovatisChannelGrp(ennovatis::ChannelGroup group) {
  switchTo(m_ennovatis, m_switch);
  m_channelGrp = std::make_shared<ennovatis::ChannelGroup>(group);

  if constexpr (debug) {
    auto &b = m_buildings->at(0);
    m_debug_worker.fetchChannels(group, b, *m_req);
  }
  updateEnnovatisChannelGrp();
}

bool EnergyPlugin::updateChannelIDsFromCSV(const std::string &pathToCSV) {
  auto csvPath = std::filesystem::path(pathToCSV);
  if (csvPath.extension() == ".csv") {
    std::ifstream csvFilestream(pathToCSV);
    if (!csvFilestream.is_open()) {
      std::cout << "File does not exist or cannot be opened: " << pathToCSV
                << std::endl;
      return false;
    }
    ennovatis::csv_channelid_parser csvParser;
    if (!csvParser.update_buildings_by_buildingid(pathToCSV, m_buildings))
      return false;
  }
  return true;
}

bool EnergyPlugin::loadChannelIDs(const std::string &pathToJSON,
                                  const std::string &pathToCSV) {
  std::ifstream inputFilestream(pathToJSON);
  if (!inputFilestream.is_open()) {
    std::cout << "File does not exist or cannot be opened: " << pathToJSON
              << std::endl;
    return false;
  }
  auto jsonPath = std::filesystem::path(pathToJSON);
  if (jsonPath.extension() == ".json") {
    ennovatis::sax_channelid_parser slp(m_buildings);
    if (!slp.parse_filestream(inputFilestream)) return false;

    if (!updateChannelIDsFromCSV(pathToCSV)) return false;

    if constexpr (debug)
      for (auto &log : slp.getDebugLogs()) std::cout << log << std::endl;
  }
  return true;
}

void EnergyPlugin::initRESTRequest() {
  m_req = std::make_shared<ennovatis::rest_request>();
  m_req->url = configString("Ennovatis", "restUrl", "default")->value();
  m_req->projEid = configString("Ennovatis", "projEid", "default")->value();
  m_req->channelId = "";
  m_req->dtf = std::chrono::system_clock::now() - std::chrono::hours(24);
  m_req->dtt = std::chrono::system_clock::now();
  m_ennovatisFrom->setValue(
      ennovatis::date::time_point_to_str(m_req->dtf, ennovatis::date::dateformat));
  m_ennovatisTo->setValue(
      ennovatis::date::time_point_to_str(m_req->dtt, ennovatis::date::dateformat));
}

std::unique_ptr<EnergyPlugin::const_buildings>
EnergyPlugin::updateEnnovatisBuildings(const DeviceList &deviceList) {
  auto lastDst = 0;
  auto noDeviceMatches = const_buildings();
  Device::ptr devicePickInteractor;
  auto fillLatLon = [&](ennovatis::Building &b) {
    b.setX(devicePickInteractor->getInfo()->lon);
    b.setY(devicePickInteractor->getInfo()->lat);
  };

  auto updateBuildingInfo = [&](ennovatis::Building &b, Device::ptr dev) {
    if (m_devBuildMap.find(dev) != m_devBuildMap.end()) return;
    m_devBuildMap[dev] = &b;
    // name in ennovatis is the street => first set it for street => then set
    // the name
    b.setStreet(b.getName());
    b.setName(dev->getInfo()->name);
    b.setHeight(dev->getInfo()->height);
    b.setArea(dev->getInfo()->flaeche);
    fillLatLon(b);
  };

  for (auto &building : *m_buildings) {
    lastDst = 100;
    devicePickInteractor = nullptr;
    const auto &ennovatis_strt = building.getName();
    for (const auto &[_, devices] : deviceList) {
      const auto &d = devices.front()->getDevice();
      const auto &device_strt = d->getInfo()->strasse;
      auto lvnstnDst = computeLevensteinDistance(ennovatis_strt, device_strt);

      // if the distance is 0, we have a perfect match
      if (!lvnstnDst) {
        lastDst = 0;
        devicePickInteractor = d;
        break;
      }

      // if the distance is less than the last distance, we have a better match
      if (lvnstnDst < lastDst) {
        lastDst = lvnstnDst;
        devicePickInteractor = d;
      }
      // if the distance is the same as the last distance, we have a better
      // match if the street number is the same
      else if (lvnstnDst == lastDst && cmpStrtNo(ennovatis_strt, device_strt)) {
        devicePickInteractor = d;
      }
    }
    if (!lastDst && devicePickInteractor) {
      updateBuildingInfo(building, devicePickInteractor);
      continue;
    }
    if (devicePickInteractor) {
      auto hit = m_devBuildMap.find(devicePickInteractor);
      if (hit == m_devBuildMap.end()) {
        updateBuildingInfo(building, devicePickInteractor);
      } else {
        noDeviceMatches.push_back(&building);
      }
    }
  }
  return std::make_unique<const_buildings>(noDeviceMatches);
}

/* #endregion */

/* #region HISTORIC */

void EnergyPlugin::reinitDevices(int comp) {
  for (auto s : m_SDlist) {
    if (s.second.empty()) continue;
    for (auto devSens : s.second) {
      auto t = devSens->getDevice();
      t->init(rad, scaleH, comp);
    }
  }
}

void EnergyPlugin::setComponent(Components c) {
  switchTo(m_sequenceList, m_switch);
  switch (c) {
    case Strom:
      StromBt->setState(true, false);
      break;
    case Waerme:
      WaermeBt->setState(true, false);
      break;
    case Kaelte:
      KaelteBt->setState(true, false);
      break;
    default:
      break;
  }
  m_selectedComp = c;
  reinitDevices(c);
}

bool EnergyPlugin::loadDB(const std::string &path, const ProjTrans &projTrans) {
  if (!loadDBFile(path, projTrans)) {
    return false;
  }

  setAnimationTimesteps(m_sequenceList->getNumChildren(), m_sequenceList);

  rad = 3.;
  scaleH = 0.1;

  reinitDevices(m_selectedComp);
  return true;
}

void EnergyPlugin::helper_initTimestepGrp(size_t maxTimesteps,
                                          osg::ref_ptr<osg::Group> &timestepGroup) {
  for (int t = 0; t < maxTimesteps; ++t) {
    timestepGroup = new osg::Group();
    std::string groupName = "timestep" + std::to_string(t);
    timestepGroup->setName(groupName);
    m_sequenceList->addChild(timestepGroup);
    m_sequenceList->setValue(t);
  }
}

void EnergyPlugin::helper_initTimestepsAndMinYear(
    size_t &maxTimesteps, int &minYear, const std::vector<std::string> &header) {
  for (const auto &h : header) {
    if (h.find("Strom") != std::string::npos) {
      auto minYearStr =
          std::regex_replace(h, std::regex("[^0-9]*"), std::string("$1"));
      int min_year_tmp = std::stoi(minYearStr);
      if (min_year_tmp < minYear) minYear = min_year_tmp;
      ++maxTimesteps;
    }
  }
}

void EnergyPlugin::helper_projTransformation(bool mapdrape, PJ *P, PJ_COORD &coord,
                                             DeviceInfo::ptr deviceInfoPtr,
                                             const double &lat, const double &lon) {
  if (!mapdrape) {
    deviceInfoPtr->lon = lon;
    deviceInfoPtr->lat = lat;
    deviceInfoPtr->height = 0.f;
    return;
  }

  // x = lon, y = lat
  coord.lpzt.lam = lon;
  coord.lpzt.phi = lat;
  float alt = 0.;

  coord = proj_trans(P, PJ_FWD, coord);

  deviceInfoPtr->lon = coord.xy.x + m_offset[0];
  deviceInfoPtr->lat = coord.xy.y + m_offset[1];
  deviceInfoPtr->height = alt + m_offset[2];
}

void EnergyPlugin::helper_handleEnergyInfo(size_t maxTimesteps, int minYear,
                                           const CSVStream::CSVRow &row,
                                           DeviceInfo::ptr deviceInfoPtr) {
  auto font = configString("Billboard", "font", "default")->value();
  for (size_t year = minYear; year < minYear + maxTimesteps; ++year) {
    auto str_yr = std::to_string(year);
    auto strom = "Strom " + str_yr;
    auto waerme = "Wärme " + str_yr;
    auto kaelte = "Kälte " + str_yr;
    auto deviceInfoTimestep = std::make_shared<energy::DeviceInfo>(*deviceInfoPtr);
    float strom_val = 0.f;
    ACCESS_CSV_ROW(row, strom, strom_val);
    ACCESS_CSV_ROW(row, waerme, deviceInfoTimestep->waerme);
    ACCESS_CSV_ROW(row, kaelte, deviceInfoTimestep->kaelte);
    deviceInfoTimestep->strom = strom_val / 1000.;  // kW -> MW
    auto timestep = year - 2000;
    deviceInfoTimestep->timestep = timestep;
    auto device = std::make_shared<energy::Device>(
        deviceInfoTimestep, m_sequenceList->getChild(timestep)->asGroup(), font);
    auto deviceSensor =
        std::make_shared<energy::DeviceSensor>(device, device->getGroup());
    m_SDlist[deviceInfoPtr->ID].push_back(deviceSensor);
  }
}

bool EnergyPlugin::loadDBFile(const std::string &fileName,
                              const ProjTrans &projTrans) {
  try {
    auto csvStream = CSVStream(fileName);
    size_t maxTimesteps = 0;
    int minYear = 2000;
    const auto &header = csvStream.getHeader();
    helper_initTimestepsAndMinYear(maxTimesteps, minYear, header);

    CSVStream::CSVRow row;

    std::string sensorType;
    osg::ref_ptr<osg::Group> timestepGroup;
    bool mapdrape = true;

    auto P = proj_create_crs_to_crs(PJ_DEFAULT_CTX, projTrans.projFrom.c_str(),
                                    projTrans.projTo.c_str(), NULL);
    PJ_COORD coord;
    coord.lpzt.z = 0.0;
    coord.lpzt.t = HUGE_VAL;

    if (!P) {
      fprintf(stderr,
              "Energy Plugin: Ignore mapping. No valid projection was "
              "found between given proj string in "
              "config EnergyCampus.toml\n");
      mapdrape = false;
    }

    helper_initTimestepGrp(maxTimesteps, timestepGroup);

    while (csvStream >> row) {
      auto deviceInfoPtr = std::make_shared<DeviceInfo>();
      auto lat = std::stod(row["lat"]);
      auto lon = std::stod(row["lon"]);

      // location
      helper_projTransformation(mapdrape, P, coord, deviceInfoPtr, lat, lon);

      deviceInfoPtr->ID = row["GebäudeID"];
      deviceInfoPtr->strasse = row["Straße"] + " " + row["Nr"];
      deviceInfoPtr->name = row["Details"];
      ACCESS_CSV_ROW(row, "Baujahr", deviceInfoPtr->baujahr);
      ACCESS_CSV_ROW(row, "Grundfläche (GIS)", deviceInfoPtr->flaeche);

      // electricity, heat, cold
      helper_handleEnergyInfo(maxTimesteps, minYear, row, deviceInfoPtr);
    }
    proj_destroy(P);
  } catch (const CSVStream_Exception &ex) {
    std::cout << ex.what() << std::endl;
    return false;
  }
  return true;
}

/* #endregion */

/* #region SIMULATION_DATA */

void EnergyPlugin::initColorMap() {
  if (m_simulationMenu == nullptr) {
    initSimMenu();
  }

  m_colorMapMenu = std::make_unique<covise::ColorMapUI>(*m_simulationMenu);
  m_colorMapMenu->setCallback(
      [this](const covise::ColorMap &cm) { updateColorMap(cm); });
}

void EnergyPlugin::updateColorMap(const covise::ColorMap &map) {
  // heating simulation
//   if (m_heatingGroup->getNodeMask()) {
    m_heatingSimUI->updateTimestepColors("mass_flow", m_colorMapMenu->getMin(),
                                         m_colorMapMenu->getMax(), true);
    auto colorMap = m_colorMapMenu->getColorMap();
    auto min_new = colorMap->min;
    auto max_new = colorMap->max;

    m_colorMapMenu->setMinBounds(min_new, max_new);
    m_colorMapMenu->setMaxBounds(min_new, max_new);
//   } else if (m_powerGroup->getNodeMask()) {
//     // do something
//   }
}

void EnergyPlugin::initSimMenu() {
  if (m_simulationMenu != nullptr) return;
  m_simulationMenu = new ui::Menu(EnergyTab, "Simulation");
  m_simulationMenu->setText("Simulation");
}

void EnergyPlugin::switchEnergyGrid(EnergyGrids grid) {
  osg::ref_ptr<osg::Group> switch_to = nullptr;
  switch (grid) {
    case PowerGrid:
      switch_to = m_powerGroup;
      break;
    case HeatingGrid:
      switch_to = m_heatingGroup;
      break;
    case CoolingGrid:
      // TODO: implement cooling grid
      std::cerr << "Cooling grid not implemented yet" << std::endl;
      return;
  }
  switchTo(switch_to, m_grid);
}

void EnergyPlugin::initEnergyGridUI() {
  if (m_simulationMenu == nullptr) {
    initSimMenu();
  }
  m_energygridGroup = new ui::Group(m_simulationMenu, "EnergyGrid");

  m_energygridBtnGroup = new ui::ButtonGroup(m_energygridGroup, "EnergyGrid");
  m_energygridBtnGroup->setDefaultValue(HeatingGrid);
  m_energygridBtnGroup->setCallback(
      [this](int value) { switchEnergyGrid(EnergyGrids(value)); });
  m_powerGridBtn =
      new ui::Button(m_simulationMenu, "PowerGrid", m_energygridBtnGroup, PowerGrid);
  m_heatingGridBtn = new ui::Button(m_simulationMenu, "HeatingGrid",
                                    m_energygridBtnGroup, HeatingGrid);
}

void EnergyPlugin::initSimUI() {
  initSimMenu();
  initEnergyGridUI();
  initColorMap();
}

void EnergyPlugin::initPowerGridStreams() {
  auto powerGridDir = configString("Simulation", "powerGridDir", "default")->value();
  fs::path dir_path(powerGridDir);
  if (!fs::exists(dir_path)) return;
  m_powerGridStreams = getCSVStreams(dir_path);
  if (!m_powerGridStreams) {
    std::cout << "No csv files found in " << powerGridDir << std::endl;
    return;
  }
}

// TODO:
// [ ] - add a button to enable/disable the simulation data
// [ ] - plan uniform grid structure file => csv file in specific format
std::unique_ptr<EnergyPlugin::FloatMap> EnergyPlugin::getInlfuxDataFromCSV(
    opencover::utils::read::CSVStream &stream, float &max, float &min, float &sum,
    int &timesteps) {
  const auto &headers = stream.getHeader();
  FloatMap values;
  if (stream && headers.size() > 1) {
    CSVStream::CSVRow row;
    while (stream >> row) {
      for (auto cityGMLBuildingName : headers) {
        auto sensor = m_cityGMLObjs.find(cityGMLBuildingName);
        if (sensor != m_cityGMLObjs.end()) {
          float value = 0;
          ACCESS_CSV_ROW(row, cityGMLBuildingName, value);
          if (value > max)
            max = value;
          else if (value < min || min == -1)
            min = value;
          sum += value;
          if (values.find(cityGMLBuildingName) == values.end())
            values.insert({cityGMLBuildingName, {value}});
          else
            values[cityGMLBuildingName].push_back(value);
        }
      }
      ++timesteps;
    }
  }
  return std::make_unique<FloatMap>(values);
}

void EnergyPlugin::applyStaticInfluxToCityGML(
    const std::string &filePathToInfluxCSV) {
  if (m_cityGMLObjs.empty()) return;
  if (!boost::filesystem::exists(filePathToInfluxCSV)) return;
  auto csvStream = CSVStream(filePathToInfluxCSV);
  float max = 0, min = -1;
  float sum = 0;
  int timesteps = 0;
  auto values = getInlfuxDataFromCSV(csvStream, max, min, sum, timesteps);

  auto colorMap = m_colorMapMenu->getColorMap();
  colorMap->max = max;
  colorMap->min = min;
  *colorMap = covise::interpolateColorMap(*colorMap, m_colorMapMenu->getNumSteps());

  auto distributionCenter = sum / (timesteps * values->size());
  m_colorMapMenu->setMinBounds(0, distributionCenter);
  m_colorMapMenu->setMaxBounds(distributionCenter, max);

  for (auto &[name, values] : *values) {
    auto sensorIt = m_cityGMLObjs.find(name);
    if (sensorIt != m_cityGMLObjs.end()) {
      sensorIt->second->updateTimestepColors(values);
    }
  }
  setAnimationTimesteps(timesteps, m_cityGML);
}

bool EnergyPlugin::checkBoxSelection_powergrid(const std::string &tableName,
                                               const std::string &paramName) {
  using namespace std::placeholders;
  auto eq_name = [](const std::string &compare,
                    const auto &pointerWithNameFunction) {
    return compare == pointerWithNameFunction->name();
  };
  auto eq_tableName = [&tableName, &eq_name](const auto &pair) {
    auto menu = pair.first;
    return eq_name(tableName, menu);
  };
  if (auto it = std::find_if(m_powerGridCheckboxes.begin(),
                             m_powerGridCheckboxes.end(), eq_tableName);
      it != m_powerGridCheckboxes.end()) {
    const auto &checkBoxes = it->second;
    if (auto it = std::find_if(checkBoxes.begin(), checkBoxes.end(),
                               std::bind(eq_name, paramName, _1));
        it != checkBoxes.end()) {
      return (*it)->state();
    }
  }
  return false;
}

void EnergyPlugin::rebuildPowerGrid() {
  initPowerGridStreams();
  buildPowerGrid();
}

void EnergyPlugin::updatePowerGridConfig(const std::string &tableName,
                                         const std::string &name, bool on) {
  int idx = 0;
  for (auto &[menuName, checkBoxes] : m_powerGridCheckboxes) {
    if (menuName->name() != tableName) {
      idx += checkBoxes.size();
      continue;
    }
    for (auto &checkBox : checkBoxes) {
      if (checkBox->name() == name) {
        (*m_powerGridSelectionPtr)[idx] = on;
        return;
      }
      ++idx;
    }
  }
}

void EnergyPlugin::updatePowerGridSelection(bool on) {
  if (!on) return;
  m_updatePowerGridSelection->setState(false);
  for (auto i = 0; i < m_Energy->getNumChildren(); ++i) {
    auto child = m_Energy->getChild(i);
    if (child->getName() == "PowerGrid") {
      m_Energy->removeChild(i);
      break;
    }
  }
  rebuildPowerGrid();
}

void EnergyPlugin::initPowerGridUI(const std::vector<std::string> &tablesToSkip) {
  if (!m_powerGridStreams) initPowerGridStreams();
  m_powerGridMenu = new opencover::ui::Menu("PowerGridData", EnergyTab);

  m_updatePowerGridSelection = new opencover::ui::Button(m_powerGridMenu, "Update");
  m_updatePowerGridSelection->setState(false);
  m_updatePowerGridSelection->setCallback([this](bool enable) {
    updatePowerGridSelection(enable);
    switchTo(m_powerGroup, m_grid);
  });

  m_powerGridSelectionPtr =
      configBoolArray("Simulation", "powerGridDataSelection", std::vector<bool>{});
  auto powerGridSelection = m_powerGridSelectionPtr->value();
  bool initConfig = false;
  if (powerGridSelection.size() < 1) initConfig = true;

  int i = 0;
  for (auto &[name, stream] : *m_powerGridStreams) {
    if (std::any_of(tablesToSkip.begin(), tablesToSkip.end(),
                    [n = name](const std::string &table) { return table == n; }))
      continue;

    auto menu = new opencover::ui::Menu(m_powerGridMenu, name);
    menu->allowRelayout(true);

    auto header = stream->getHeader();
    std::vector<opencover::ui::Button *> checkBoxMap;
    for (auto &col : header) {
      if (col.find("Unnamed") == 0) continue;
      auto checkBox = new opencover::ui::Button(menu, col);
      checkBox->setCallback([this, tableName = name, col](bool on) {
        updatePowerGridConfig(tableName, col, on);
      });
      if (initConfig) {
        checkBox->setState(true);
        powerGridSelection.push_back(true);
      } else {
        checkBox->setState(powerGridSelection[i]);
      }
      //   checkBoxMap.insert({col, checkBox});
      checkBoxMap.push_back(checkBox);
      ++i;
    }
    if (auto it = m_powerGridCheckboxes.find(menu);
        it != m_powerGridCheckboxes.end()) {
      auto &[_, chBxMap] = *it;
      //   chBxMap.insert(checkBoxMap.begin(), checkBoxMap.end());
      std::move(checkBoxMap.begin(), checkBoxMap.end(), std::back_inserter(chBxMap));
    } else {
      m_powerGridCheckboxes.emplace(menu, checkBoxMap);
    }
  }
  if (initConfig) {
    m_powerGridSelectionPtr->resize(powerGridSelection.size());
    for (auto i = 0; i < powerGridSelection.size(); ++i)
      (*m_powerGridSelectionPtr)[i] = powerGridSelection[i];
  }
}

void EnergyPlugin::applySimulationDataToPowerGrid() {
    // only data for citygml
}

void EnergyPlugin::initPowerGrid() {
  initPowerGridStreams();
  initPowerGridUI({"trafo3w_std_types", "trafo_std_types", "trafo", "parameters",
                   "dtypes", "bus_geodata", "fuse_std_types", "line_std_types"});
  buildPowerGrid();
  applySimulationDataToPowerGrid();
  m_powerGridStreams->clear();
}

void EnergyPlugin::initGrid() {
  initPowerGrid();
  initHeatingGrid();
  buildCoolingGrid();
}

std::unique_ptr<std::vector<std::string>> EnergyPlugin::getBusNames(
    opencover::utils::read::CSVStream &stream) {
  auto busNames = std::vector<std::string>();
  CSVStream::CSVRow bus;
  std::string busName("");
  while (stream >> bus) {
    ACCESS_CSV_ROW(bus, "name", busName);
    busNames.push_back(busName);
  }
  return std::make_unique<std::vector<std::string>>(busNames);
}

void EnergyPlugin::helper_getAdditionalPowerGridPointData_addData(
    int busId, core::simulation::grid::DataList &additionalData,
    const core::simulation::grid::Data &data) {
  if (busId > -1 && busId < additionalData.size()) {
    auto &existingData = additionalData[busId];
    if (existingData.empty())
      additionalData[busId] = data;
    else
      existingData.insert(data.begin(), data.end());
  }
}

void EnergyPlugin::helper_getAdditionalPowerGridPointData_handleDuplicate(
    std::string &name, std::map<std::string, uint> &duplicateMap) {
  if (auto it = duplicateMap.find(name); it != duplicateMap.end())
    // if there is a similar entity, add the id to the name
    name = name + "_" + std::to_string(++it->second);
  else
    duplicateMap.insert({name, 0});
}

std::unique_ptr<core::simulation::grid::DataList>
EnergyPlugin::getAdditionalPowerGridPointData(const std::size_t &numOfBus) {
  using DataList = core::simulation::grid::DataList;

  // additional bus data
  DataList additionalData(numOfBus);

  for (auto &[tableName, tableStream] : *m_powerGridStreams) {
    auto header = tableStream->getHeader();
    if (auto it = std::find(header.begin(), header.end(), "bus"); it == header.end())
      continue;
    auto it = std::find(header.begin(), header.end(), "bus");
    if (it == header.end()) CSVStream::CSVRow busdata;
    int busId = -1;
    std::map<std::string, uint> duplicate{};
    CSVStream::CSVRow row;
    // row
    while (*tableStream >> row) {
      core::simulation::grid::Data data;
      // column
      for (auto &colName : header) {
        if (!checkBoxSelection_powergrid(tableName, colName)) continue;
        // get bus id without adding it
        if (colName == "bus") {
          ACCESS_CSV_ROW(row, colName, busId);
          continue;
        }
        std::string value;
        ACCESS_CSV_ROW(row, colName, value);

        // add the name of the table to the name
        std::string columnNameWithTable = tableName + " > " + colName;
        helper_getAdditionalPowerGridPointData_handleDuplicate(columnNameWithTable,
                                                               duplicate);
        data[columnNameWithTable] = value;
      }
      helper_getAdditionalPowerGridPointData_addData(busId, additionalData, data);
    }
  }
  return std::make_unique<DataList>(additionalData);
}

std::unique_ptr<core::simulation::grid::Points> EnergyPlugin::createPowerGridPoints(
    opencover::utils::read::CSVStream &stream, const float &sphereRadius,
    const std::vector<std::string> &busNames) {
  using Points = core::simulation::grid::Points;

  CSVStream::CSVRow point;
  float lat = 0, lon = 0;
  Points points;
  int busID = 0;

  auto additionalData = getAdditionalPowerGridPointData(busNames.size());

  while (stream >> point) {
    ACCESS_CSV_ROW(point, "x", lon);
    ACCESS_CSV_ROW(point, "y", lat);

    // x = lon, y = lat
    projTransLatLon(lat, lon);

    auto busName = busNames[busID];
    auto busData = (*additionalData)[busID];

    osg::ref_ptr<core::simulation::grid::Point> p =
        new core::simulation::grid::Point(busName, lon, lat, 0.0f, sphereRadius,
                                          busData);
    points.push_back(p);
    ++busID;
  }
  return std::make_unique<Points>(points);
}

std::pair<std::unique_ptr<core::simulation::grid::Indices>,
          std::unique_ptr<core::simulation::grid::DataList>>
EnergyPlugin::getPowerGridIndicesAndOptionalData(
    opencover::utils::read::CSVStream &stream, const size_t &numBus) {
  using Indices = core::simulation::grid::Indices;
  using DataList = core::simulation::grid::DataList;
  Indices indices(numBus);
  CSVStream::CSVRow line;
  int from = 0, to = 0;
  core::simulation::grid::DataList additionalData;
  // TODO: add window for user to select the columns
  auto header = stream.getHeader();
  while (stream >> line) {
    core::simulation::grid::Data data;

    for (auto colName : header) {
      fs::path filename(stream.getFilename());
      auto filename_without_ext = filename.stem().string();
      if (!checkBoxSelection_powergrid(filename_without_ext, colName)) continue;
      std::string value;
      ACCESS_CSV_ROW(line, colName, value);
      data[colName] = value;
    }

    ACCESS_CSV_ROW(line, "from_bus", from);
    ACCESS_CSV_ROW(line, "to_bus", to);

    indices[from].push_back(to);
    additionalData.push_back(data);
  }
  return std::make_pair(std::make_unique<Indices>(indices),
                        std::make_unique<DataList>(additionalData));
}

void EnergyPlugin::buildPowerGrid() {
  if (!m_powerGridStreams) return;

  // fetch bus names
  auto busData = m_powerGridStreams->find("bus");
  std::unique_ptr<std::vector<std::string>> busNames(nullptr);
  if (busData != m_powerGridStreams->end()) {
    auto &[name, busStream] = *busData;
    busNames = getBusNames(*busStream);
  }

  if (!busNames) return;

  // create points
  auto pointsData = m_powerGridStreams->find("bus_geodata");
  std::unique_ptr<core::simulation::grid::Points> points(nullptr);
  int busId(0);
  float sphereRadius(1.0f);
  if (pointsData != m_powerGridStreams->end()) {
    auto &[name, pointStream] = *pointsData;
    points = createPowerGridPoints(*pointStream, sphereRadius, *busNames);
  }

  // create line
  auto lineData = m_powerGridStreams->find("line");
  std::unique_ptr<core::simulation::grid::Indices> indices = nullptr;
  std::unique_ptr<core::simulation::grid::DataList> optData = nullptr;
  if (lineData != m_powerGridStreams->end()) {
    auto &[name, lineStream] = *lineData;
    std::tie(indices, optData) =
        getPowerGridIndicesAndOptionalData(*lineStream, busNames->size());
  }

  // create grid
  if (indices == nullptr || points == nullptr) return;

  m_powerGroup = new osg::Group();
  auto font = configString("Billboard", "font", "default")->value();
  TxtBoxAttributes infoboardAttributes = TxtBoxAttributes(
      osg::Vec3(0, 0, 0), "EnergyGridText", font, 50, 50, 2.0f, 0.1, 2);
  m_powerGroup->setName("PowerGrid");
  //   m_powerGrid = std::make_unique<core::EnergyGrid>(core::EnergyGridConfig{
  m_powerGrid = std::make_shared<EnergyGrid>(
      EnergyGridConfig{"POWER", *points, *indices, m_powerGroup, 0.5f, *optData,
                       infoboardAttributes});
  m_powerGrid->initDrawables();
  m_powerGrid->updateColor(
      osg::Vec4(255.0f / 255.0f, 222.0f / 255.0f, 33.0f / 255.0f, 1.0f));
  //   m_Energy->addChild(powerGroup);
  m_grid->addChild(m_powerGroup);

  // TODO:
  //  [ ] set trafo as 3d model or block

  // how to implement this generically?
  // - fixed grid structure for discussion in AK Software
  // - look into Energy ADE
}

void EnergyPlugin::initHeatingGridStreams() {
  auto heatingGridDir =
      configString("Simulation", "heatingGridDir", "default")->value();
  fs::path dir_path(heatingGridDir);
  if (!fs::exists(dir_path)) return;
  m_heatingGridStreams = getCSVStreams(dir_path);
  if (!m_heatingGridStreams) {
    std::cout << "No csv files found in " << heatingGridDir << std::endl;
    return;
  }
}

void EnergyPlugin::initHeatingGrid() {
  initHeatingGridStreams();
  buildHeatingGrid();
  applySimulationDataToHeatingGrid();
  m_heatingGridStreams->clear();
}

std::vector<int> EnergyPlugin::createHeatingGridIndices(
    const std::string &pointName,
    const std::string &connectionsStrWithCommaDelimiter,
    core::simulation::grid::DataList &additionalConnectionData) {
  std::vector<int> connectivityList{};
  std::stringstream ss(connectionsStrWithCommaDelimiter);
  std::string connection("");

  while (std::getline(ss, connection, ' ')) {
    if (connection.empty() || connection == INVALID_CELL_VALUE) continue;
    core::simulation::grid::Data connectionData{
        {"name", pointName + "_" + connection}};
    additionalConnectionData.emplace_back(connectionData);
    connectivityList.push_back(std::stoi(connection));
  }
  return connectivityList;
}

void EnergyPlugin::readSimulationDataStream(
    opencover::utils::read::CSVStream &heatingSimStream) {
  if (m_heatingGrid == nullptr) return;
  std::regex consumer_value_split_regex("Consumer_(\\d+)_(.+)");
  std::regex producer_value_split_regex("Producer_(\\d+)_(.+)");
  std::smatch match;

  CSVStream::CSVRow row;
  m_heatingSim = std::make_shared<core::simulation::heating::HeatingSimulation>();
  const auto &header = heatingSimStream.getHeader();
  auto &consumers = m_heatingSim->Consumers();
  auto &producers = m_heatingSim->Producers();
  double val = 0.0f;
  std::string name(""), valName("");
  while (heatingSimStream >> row) {
    for (const auto &col : header) {
      ACCESS_CSV_ROW(row, col, val);
      if (std::regex_search(col, match, consumer_value_split_regex)) {
        name = match[1];
        valName = match[2];
        consumers.add(name);
        consumers.addData(name, valName, val);
      } else if (std::regex_search(col, match, producer_value_split_regex)) {
        name = match[1];
        valName = match[2];
        producers.add(name);
        producers.addData(name, valName, val);
      } else {
        if (val == 0) continue;
        m_heatingSim->addData(col, val);
      }
    }
  }
  m_heatingSimUI =
      std::make_unique<HeatingSimulationUI>(m_heatingSim, m_heatingGrid,
                                            m_colorMapMenu->getColorMap());
  m_heatingSimUI->updateTimestepColors("mass_flow");

  auto timesteps = m_heatingSim->getTimesteps("mass_flow");
  std::cout << "Number of timesteps: " << timesteps << std::endl;
  setAnimationTimesteps(timesteps, m_heatingGroup);
}

void EnergyPlugin::applySimulationDataToHeatingGrid() {
  if (!m_heatingGridStreams) return;
  auto simulationData = m_heatingGridStreams->find("results");
  if (simulationData == m_heatingGridStreams->end()) return;

  auto &[_, stream] = *simulationData;
  readSimulationDataStream(*stream);
}

void EnergyPlugin::readHeatingGridStream(CSVStream &heatingStream) {
  CSVStream::CSVRow row;
  //   int id = 0;
  int maxId = 0;
  core::simulation::grid::Points points{};
  core::simulation::grid::Indices indices{};
  core::simulation::grid::DataList additionalConnectionData{};
  core::simulation::grid::Data pointData{};
  std::map<int, int> idMap{};
  m_heatingGroup = new osg::Group();
  auto font = configString("Billboard", "font", "default")->value();
  TxtBoxAttributes infoboardAttributes = TxtBoxAttributes(
      osg::Vec3(0, 0, 0), "EnergyGridText", font, 50, 50, 2.0f, 0.1, 2);

  auto checkForInvalidValue = [](const std::string &value) {
    return value == INVALID_CELL_VALUE;
  };

  auto addToPointData = [&checkForInvalidValue](
                            core::simulation::grid::Data &pointData,
                            const std::string &key, const std::string &value) {
    if (!checkForInvalidValue(value)) pointData[key] = value;
  };
  std::string name = "", connections = "", label = "", type = "";
  float lat = 0.0f, lon = 0.0f;

  int idx = 0;
  while (heatingStream >> row) {
    ACCESS_CSV_ROW(row, "connections", connections);
    ACCESS_CSV_ROW(row, "id", name);
    ACCESS_CSV_ROW(row, "Latitude", lat);
    ACCESS_CSV_ROW(row, "Longitude", lon);
    ACCESS_CSV_ROW(row, "Label", label);
    ACCESS_CSV_ROW(row, "Type", type);

    addToPointData(pointData, "name", name);
    addToPointData(pointData, "label", label);
    addToPointData(pointData, "type", type);

    projTransLatLon(lat, lon);

    int strangeId = std::stoi(name);
    if (maxId < strangeId) maxId = strangeId;

    // create a point
    osg::ref_ptr<core::simulation::grid::Point> point =
        new core::simulation::grid::Point(name, lon, lat, 0.0f, 1.0f, pointData);
    points.push_back(point);
    idMap[strangeId] = idx;

    // create and add indice
    indices.emplace_back(
        createHeatingGridIndices(name, connections, additionalConnectionData));

    ++idx;

    // needs cleanup because dataset is not final and has empty cells => no need to
    // display them
    pointData.clear();
    row.clear();
  }

  // indices are strangely mapped with random ids => make them contiguous
  for (int i = 0; i < indices.size(); ++i)
    for (int j = 0; j < indices[i].size(); ++j)
      if (auto it = idMap.find(indices[i][j]); it != idMap.end())
        indices[i][j] = it->second;

  m_heatingGroup->setName("HeatingGrid");
  m_heatingGrid = std::make_unique<EnergyGrid>(
      EnergyGridConfig{"HEATING", points, indices, m_heatingGroup, 0.5f,
                       additionalConnectionData, infoboardAttributes});
  m_heatingGrid->initDrawables();
  m_heatingGrid->updateColor(
      osg::Vec4(168.0f / 255.0f, 50.0f / 255.0f, 50.0f / 255.0f, 1.0f));
  //   m_Energy->addChild(heatingGroup);
  m_grid->addChild(m_heatingGroup);
  switchTo(m_heatingGroup, m_grid);
}

void EnergyPlugin::buildHeatingGrid() {
  if (!m_heatingGridStreams) return;

  // find correct csv
  auto heatingIt = m_heatingGridStreams->find("heating_network_2");
  if (heatingIt == m_heatingGridStreams->end()) return;

  auto &[_, heatingStream] = *heatingIt;
  readHeatingGridStream(*heatingStream);
}

void EnergyPlugin::buildCoolingGrid() {
  // NOTE: implement when data is available
}

/* #endregion */
COVERPLUGIN(EnergyPlugin)
