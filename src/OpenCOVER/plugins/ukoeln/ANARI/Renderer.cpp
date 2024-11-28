/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <sstream>
#ifdef __GNUC__
#include <execinfo.h>
#include <sys/time.h>
#endif
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif
#ifdef ANARI_PLUGIN_HAVE_MPI
#include <mpi.h>
#endif
#include <anari/anari_cpp.hpp>
#include <anari/anari_cpp/ext/glm.h>
//#include <glm/gtx/string_cast.hpp>
#include <osg/io_utils>
#include <config/CoviseConfig.h>
#include <cover/coVRConfig.h>
#include <cover/coVRLighting.h>
#include <cover/coVRPluginSupport.h>
#ifdef ANARI_PLUGIN_HAVE_CUDA
#include <PluginUtil/CudaSafeCall.h>
#endif
#include "generateRandomSpheres.h"
#include "readPTS.h"
#include "readPLY.h"
#include "Projection.h"
#include "Renderer.h"
#include "hdri.h"

using namespace covise;
using namespace opencover;

// #define TIMING

#define OU_VIEWPORT_UPDATED      0x1
#define OU_CAMERA_UPDATED        0x2
#define OU_BOUNDS_UPDATED        0x4
#define OU_ANIMATION_UPDATED     0x8
#define OU_TRANSFUNC_UPDATED    0x10
#define OU_APP_PARAMS_UPDATED   0x20
#define OU_ANARI_PARAMS_UPDATED 0x40

void statusFunc(const void *userData,
    ANARIDevice device,
    ANARIObject source,
    ANARIDataType sourceType,
    ANARIStatusSeverity severity,
    ANARIStatusCode code,
    const char *message)
{
    (void)userData;
    if (severity == ANARI_SEVERITY_FATAL_ERROR)
        fprintf(stderr, "[FATAL] %s\n", message);
    else if (severity == ANARI_SEVERITY_ERROR)
        fprintf(stderr, "[ERROR] %s\n", message);
    else if (severity == ANARI_SEVERITY_WARNING)
        fprintf(stderr, "[WARN ] %s\n", message);
    else if (severity == ANARI_SEVERITY_PERFORMANCE_WARNING)
        fprintf(stderr, "[PERF ] %s\n", message);
    else if (severity == ANARI_SEVERITY_INFO)
        fprintf(stderr, "[INFO] %s\n", message);
}

inline double getCurrentTime()
{
#ifdef _WIN32
  SYSTEMTIME tp; GetSystemTime(&tp);
  /*
     Please note: we are not handling the "leap year" issue.
 */
  size_t numSecsSince2020
      = tp.wSecond
      + (60ull) * tp.wMinute
      + (60ull * 60ull) * tp.wHour
      + (60ull * 60ul * 24ull) * tp.wDay
      + (60ull * 60ul * 24ull * 365ull) * (tp.wYear - 2020);
  return double(numSecsSince2020 + tp.wMilliseconds * 1e-3);
#else
  struct timeval tp; gettimeofday(&tp,nullptr);
  return double(tp.tv_sec) + double(tp.tv_usec)/1E6;
#endif
}

static std::vector<std::string> string_split(std::string s, char delim)
{
    std::vector<std::string> result;

    std::istringstream stream(s);

    for (std::string token; std::getline(stream, token, delim); )
    {
        result.push_back(token);
    }

    return result;
}

static std::string getExt(const std::string &fileName)
{
    int pos = fileName.rfind('.');
    if (pos == fileName.npos)
        return "";
    return fileName.substr(pos);
}

enum FileType {
    OBJ,
    UMesh,
    UMeshScalars,
    VTU,
    VTK,
    Unknown,
    // Keep last:
    FileTypeCount,
};

inline
FileType getFileType(const std::string &fileName)
{
    auto ext = getExt(fileName);
    if (ext == ".obj") return OBJ;
    else if (ext == ".umesh") return UMesh;
    else if (ext == ".scalars") return UMeshScalars;
    else if (ext == ".floats") return UMeshScalars;
    else if (ext == ".vtu") return VTU;
    else if (ext == ".vtk") return VTK;
    else return Unknown;
}

inline
int getID(std::string fileName) {
    // check if we know this file name:
    static std::map<std::string,int> knownFileNames[FileTypeCount];
    FileType type = getFileType(fileName);

    auto it = knownFileNames[type].find(fileName);
    if (it != knownFileNames[type].end()) {
      return it->second;
    }

    // file name is unknown
    static int nextID[FileTypeCount] = { 0 };
    int ID = nextID[type]++;
    knownFileNames[type][fileName] = ID;
    return ID;
}

inline
bool isLanderUMesh(std::string fileName) {
    return getFileType(fileName) == UMesh
        && fileName.find("lander-small") != std::string::npos;
}

inline
bool isLanderScalars(std::string fileName) {
    return getFileType(fileName) == UMeshScalars
        && fileName.find("lander-small__var_") != std::string::npos;
}

struct Slot {
    Slot() = default;
    Slot(std::string fileName, int mpiSize) {
        namespace fs = std::filesystem;
        std::string base = fs::path(fileName).filename().string();

        // special handling for file names we recognize:
        if (isLanderUMesh(fileName)) {
            auto splt = string_split(base, '.');
            int landerRank = std::stoi(splt[1]);
            if (landerRank <= 0 || landerRank > 72) {
                std::cerr << "Failed parsing rank from file: " << fileName << '\n';
            } else {
                mpiRank = (landerRank-1)%mpiSize;
                localID = (landerRank-1)/mpiSize;
                std::cout << "Assigning " << fileName
                          << " to rank " << mpiRank
                          << ", localID: " << localID
                          << '\n';
            }
        } else if (isLanderScalars(fileName)) {
            auto splt = string_split(base, '.');
            int landerRank = std::stoi(splt[1]);
            size_t pos = splt[0].find("ts_");
            std::string tsString = splt[0].substr(pos+3);
            if (!tsString.empty()) {
                timeStep = (std::stoi(tsString)-6200)/200;
            }
            if (landerRank <= 0 || landerRank > 72) {
                std::cerr << "Failed parsing rank from file: " << fileName << '\n';
            } else {
                mpiRank = (landerRank-1)%mpiSize;
                localID = (landerRank-1)/mpiSize;
                std::cout << "Assigning " << fileName
                          << " to rank " << mpiRank
                          << ", localID: " << localID
                          << ", timeStep: " << timeStep
                          << '\n';
            }
        } else { // not recognized: round-robin
          int ID = getID(fileName);
          mpiRank = ID%mpiSize;
        }
    }
    
    // Rank the file is assigned to
    int mpiRank{0};

    // ID of the file, across all files assigned to this rank
    int localID{0};

    // Time step
    int timeStep{0};
};


static bool deviceHasExtension(anari::Library library,
    const std::string &deviceSubtype,
    const std::string &extName)
{
    const char **extensions =
        anariGetDeviceExtensions(library, deviceSubtype.c_str());

    if (!extensions)
        return false;

    for (; *extensions; extensions++) {
        if (*extensions == extName)
            return true;
    }
    return false;
}

inline glm::mat4 osg2glm(const osg::Matrix &m)
{
    glm::mat4 res;
    // glm matrices are column-major, osg matrices are row-major!
    res[0] = glm::vec4(m(0,0), m(0,1), m(0,2), m(0,3));
    res[1] = glm::vec4(m(1,0), m(1,1), m(1,2), m(1,3));
    res[2] = glm::vec4(m(2,0), m(2,1), m(2,2), m(2,3));
    res[3] = glm::vec4(m(3,0), m(3,1), m(3,2), m(3,3));
    return res;
}

inline osg::Matrix glm2osg(const glm::mat4 &m)
{
    osg::Matrix res;
    // glm matrices are column-major, osg matrices are row-major!
    res(0,0) = m[0].x; res(0,1) = m[0].y; res(0,2) = m[0].z; res(0,3) = m[0].w;
    res(1,0) = m[1].x; res(1,1) = m[1].y; res(1,2) = m[1].z; res(1,3) = m[1].w;
    res(2,0) = m[2].x; res(2,1) = m[2].y; res(2,2) = m[2].z; res(2,3) = m[2].w;
    res(3,0) = m[3].x; res(3,1) = m[3].y; res(3,2) = m[3].z; res(3,3) = m[3].w;
    return res;
}

inline glm::vec3 randomColor(unsigned idx)
{
  unsigned int r = (unsigned int)(idx*13*17 + 0x234235);
  unsigned int g = (unsigned int)(idx*7*3*5 + 0x773477);
  unsigned int b = (unsigned int)(idx*11*19 + 0x223766);
  return glm::vec3((r&255)/255.f,
                   (g&255)/255.f,
                   (b&255)/255.f);
}

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
#ifdef ANARI_PLUGIN_HAVE_CUDA
    if (anari.cudaInterop.enabled) {
        CUDA_SAFE_CALL(cudaStreamDestroy(anari.cudaInterop.copyStream));
    }
#endif
    if (multiChannelDrawer) {
        cover->getScene()->removeChild(multiChannelDrawer);
    }
}

void Renderer::init()
{
    initMPI();
    initRR();
    initChannels();
    initDevice();

    if (!anari.device)
        throw std::runtime_error("Could not init ANARI device");

    // Try loading HDRI image from config
    bool hdriEntryExists = false;
    std::string hdriName  = covise::coCoviseConfig::getEntry(
        "value",
        "COVER.Plugin.ANARI.hdri",
        &hdriEntryExists
    );

    if (hdriEntryExists) {
        loadHDRI(hdriName);
    }

    // generate default TF:
    generateTransFunc();
}

void Renderer::loadMesh(std::string fn)
{
    Slot slot(fn, mpiSize);
    if (slot.mpiRank != mpiRank) {
        bounds.updated = true; // all ranks participate in Bcast
        return;
    }

    // deferred!
    meshData.fileName = fn;
    meshData.updated = true;
}

void Renderer::unloadMesh(std::string fn)
{
    // NO!
}

void Renderer::loadVolume(const void *data, int sizeX, int sizeY, int sizeZ, int bpc,
                          float minValue, float maxValue)
{
    // deferred!
    structuredVolumeData.data = data;
    structuredVolumeData.sizeX = sizeX;
    structuredVolumeData.sizeY = sizeY;
    structuredVolumeData.sizeZ = sizeZ;
    structuredVolumeData.bpc = bpc;
    structuredVolumeData.minValue = minValue;
    structuredVolumeData.maxValue = maxValue;
    structuredVolumeData.updated = true;
}

void Renderer::unloadVolume()
{
    // NO!
}

void Renderer::loadFLASH(std::string fn)
{
#ifdef HAVE_HDF5
    Slot slot(fn, mpiSize);
    if (slot.mpiRank != mpiRank) {
        bounds.updated = true; // all ranks participate in Bcast
        return;
    }

    // deferred!
    amrVolumeData.fileName = fn;
    amrVolumeData.updated = true;

    if (amrVolumeData.flashReader.open(fn.c_str())) {
        amrVolumeData.data = amrVolumeData.flashReader.getField(0);
    }
#endif
}

void Renderer::unloadFLASH(std::string fn)
{
    // NO!
}

void Renderer::loadUMesh(const float *vertexPosition, const uint64_t *cellIndex, const uint64_t *index,
                         const uint8_t *cellType, const float *vertexData, size_t numCells, size_t numIndices,
                         size_t numVerts, float minValue, float maxValue)
{
    // deferred!
    unstructuredVolumeData.dataSource.vertexPosition.resize(numVerts*3);
    memcpy(unstructuredVolumeData.dataSource.vertexPosition.data(),vertexPosition,numVerts*3*sizeof(float));

    unstructuredVolumeData.dataSource.cellIndex.resize(numCells);
    memcpy(unstructuredVolumeData.dataSource.cellIndex.data(),cellIndex,numCells*sizeof(uint64_t));

    unstructuredVolumeData.dataSource.index.resize(numIndices);
    memcpy(unstructuredVolumeData.dataSource.index.data(),index,numIndices*sizeof(uint64_t));

    unstructuredVolumeData.dataSource.cellType.resize(numCells);
    memcpy(unstructuredVolumeData.dataSource.cellType.data(),cellType,numCells*sizeof(uint64_t));

    unstructuredVolumeData.dataSource.vertexData.resize(numVerts);
    memcpy(unstructuredVolumeData.dataSource.vertexData.data(),vertexData,numVerts*sizeof(float));

    unstructuredVolumeData.dataSource.dataRange.x = minValue;
    unstructuredVolumeData.dataSource.dataRange.y = maxValue;
    unstructuredVolumeData.updated = true;
}

void Renderer::unloadUMesh()
{
    // NO!
}

void Renderer::loadUMeshFile(std::string fn)
{
#ifdef HAVE_UMESH
    Slot slot(fn, mpiSize);
    if (slot.mpiRank != mpiRank) {
        bounds.updated = true; // all ranks participate in Bcast
        return;
    }

    // deferred!
    if (unstructuredVolumeData.umeshReader.open(fn.c_str(), slot.localID)) {
        unstructuredVolumeData.fileName = fn;
        unstructuredVolumeData.readerType = UMESH;
        unstructuredVolumeData.updated = true;
    }
#endif
}

void Renderer::unloadUMeshFile(std::string fn)
{
    // NO!
}

void Renderer::loadUMeshScalars(std::string fn)
{
#ifdef HAVE_UMESH
    Slot slot(fn, mpiSize);
    if (slot.mpiRank != mpiRank) {
        bounds.updated = true; // all ranks participate in Bcast
        return;
    }

    unstructuredVolumeData.umeshScalarFiles.push_back({fn, 0, slot.localID, slot.timeStep});
#endif
}

void Renderer::unloadUMeshScalars(std::string fn)
{
    // NO!
}

void Renderer::loadUMeshVTK(std::string fn)
{
#ifdef HAVE_VTK
    // deferred!
    if (unstructuredVolumeData.vtkReader.open(fn.c_str())) {
        unstructuredVolumeData.fileName = fn;
        unstructuredVolumeData.readerType = VTK;
        unstructuredVolumeData.updated = true;
    }
#endif
}

void Renderer::unloadUMeshVTK(std::string fn)
{
    // NO!
}

void Renderer::loadPointCloud(std::string fn)
{
    Slot slot(fn, mpiSize);
    if (slot.mpiRank != mpiRank) {
        bounds.updated = true; // all ranks participate in Bcast
        return;
    }

    // deferred!
    pointCloudData.fileNames.push_back(fn);
    pointCloudData.updated = true;
}

void Renderer::unloadPointCloud(std::string fn)
{
    // NO!
}

void Renderer::loadHDRI(std::string fn)
{
    HDRI img;
    img.load(fn);
    hdri.pixels.resize(img.width * img.height);
    hdri.width = img.width;
    hdri.height = img.height;
    if (img.numComponents == 3) {
      memcpy(hdri.pixels.data(), img.pixel.data(), sizeof(hdri.pixels[0]) * hdri.pixels.size());
    } else if (img.numComponents == 4) {
      for (size_t i = 0; i < img.pixel.size(); i += 4) {
        hdri.pixels[i / 4] = glm::vec3(img.pixel[i], img.pixel[i + 1], img.pixel[i + 2]);
      }
    }
    hdri.updated = true;
}

void Renderer::unloadHDRI(std::string fn)
{
    // NO!
}

void Renderer::setRendererType(std::string type)
{
    if (anari.frames.empty())
        return;

    anari.renderertype = type;
    for (size_t i=0; i<channelInfos.size(); ++i) {
        // Causes frame re-initialization
        channelInfos[i].frame.width = 1;
        channelInfos[i].frame.height = 1;
        channelInfos[i].mv = glm::mat4();
        channelInfos[i].pr = glm::mat4();
    }
    initFrames();
    //anariRelease(anari.device, anari.renderer);
    //anari.renderer = nullptr;
    //anari.frames.clear();
}

std::vector<std::string> Renderer::getRendererTypes()
{
    if (rendererTypes.empty()) {
        const char * const *rendererSubtypes = nullptr;
        anariGetProperty(anari.device, anari.device, "subtypes.renderer", ANARI_STRING_LIST,
                         &rendererSubtypes, sizeof(rendererSubtypes), ANARI_WAIT);
        if (rendererSubtypes != nullptr) {

            while (const char* rendererType = *rendererSubtypes++) {
                rendererTypes.push_back(rendererType);
            }
        }

        if (rendererTypes.empty()) {
            // If the device does not support the "subtypes.renderer" property,
            // try to obtain the renderer types from the library directly
            const char** deviceSubtypes = anariGetDeviceSubtypes(anari.library);
            if (deviceSubtypes != nullptr) {
                while (const char* dstype = *deviceSubtypes++) {
                    const char** rt = anariGetObjectSubtypes(anari.device, ANARI_RENDERER);
                    while (rt && *rt) {
                        const char* rendererType = *rt++;
                        rendererTypes.push_back(rendererType);
                    }
                }
            }
        }
    }

    if (rendererTypes.empty())
        rendererTypes.push_back("default");

    return rendererTypes;
}

std::vector<ui_anari::ParameterList> &Renderer::getRendererParameters()
{
    if (rendererParameters.empty()) {
        auto r_subtypes = getRendererTypes();
        for (auto subtype : r_subtypes) {
            auto parameters =
                ui_anari::parseParameters(anari.device, ANARI_RENDERER, subtype.c_str());
            rendererParameters.push_back(parameters);
        }
    }

    return rendererParameters;
}

void Renderer::setParameter(std::string name, bool value)
{
    anari::setParameter(anari.device, anari.renderer, name.c_str(), value);
    anariCommitParameters(anari.device, anari.renderer);
}

void Renderer::setParameter(std::string name, int value)
{
    anari::setParameter(anari.device, anari.renderer, name.c_str(), value);
    anariCommitParameters(anari.device, anari.renderer);
}

void Renderer::setParameter(std::string name, float value)
{
    anari::setParameter(anari.device, anari.renderer, name.c_str(), value);
    anariCommitParameters(anari.device, anari.renderer);
}

void Renderer::loadVolumeRAW(std::string fn)
{
    // deferred!

    // parse dimensions
    std::vector<std::string> strings = string_split(fn, '_');

    int sizeX=0, sizeY=0, sizeZ=0;
    for (auto str : strings) {
        int res = sscanf(str.c_str(), "%dx%dx%dx", &sizeX, &sizeY, &sizeZ);
        if (res == 3)
            break;
    }

    size_t numVoxels = sizeX*size_t(sizeY)*sizeZ;
    if (numVoxels == 0)
        return;

    structuredVolumeData.fileName = fn;
    structuredVolumeData.data = new uint8_t[numVoxels];
    structuredVolumeData.sizeX = sizeX;
    structuredVolumeData.sizeY = sizeY;
    structuredVolumeData.sizeZ = sizeZ;
    structuredVolumeData.bpc = 1;// !
    structuredVolumeData.minValue = 0.f;
    structuredVolumeData.maxValue = 1.f;
    structuredVolumeData.updated = true;
    structuredVolumeData.deleteData = true;

    FILE *file = fopen(fn.c_str(), "rb");
    size_t res = fread((void *)structuredVolumeData.data, numVoxels, 1, file);
    if (res != 1) {
        printf("Error reading %" PRIu64 " voxels with fread(). Result was %" PRIu64 "\n",
               (uint64_t)numVoxels, (uint64_t)res);
    }
    fclose(file);
}

void Renderer::unloadVolumeRAW(std::string fn)
{
    // NO!
}

void Renderer::expandBoundingSphere(osg::BoundingSphere &bs)
{
    AABB bounds = this->bounds.global;

    osg::Vec3f minCorner(bounds.data[0],bounds.data[1],bounds.data[2]);
    osg::Vec3f maxCorner(bounds.data[3],bounds.data[4],bounds.data[5]);

    osg::Vec3f center = (minCorner+maxCorner)*.5f;
    float radius = (center-minCorner).length();
    bs.set(center, radius);
}

void Renderer::setTimeStep(int timeStep)
{
    animation.timeStep = timeStep;
    animation.updated = true;
    objectUpdates |= OU_ANIMATION_UPDATED;
}

int Renderer::getNumTimeSteps() const
{
    return animation.numTimeSteps;
}

void Renderer::updateLights(const osg::Matrix &modelMat)
{
    std::vector<Light> newLights;

    // assemble new light list
    for (size_t l=0; l<opencover::coVRLighting::instance()->lightList.size(); ++l) {
        auto &light = opencover::coVRLighting::instance()->lightList[l];
        if (light.on) {
            newLights.push_back(Light(light));
        }
    }

    // check if lights have updated
    if (newLights.size() != lights.data.size()) {
        lights.data = newLights;
        lights.updated = true;
    } else {
        for (size_t l=0; l<newLights.size(); ++l) {
            if (newLights[l].coLight.source != lights.data[l].coLight.source ||
                newLights[l].coLight.root != lights.data[l].coLight.root) {
                lights.data = newLights;
                lights.updated = true;
                break;
            }
        }
    }

    if (lights.updated || hdri.updated) {
        anari.lights.clear();

        if (!hdri.pixels.empty()) {
            ANARILight al = anariNewLight(anari.device,"hdri");

            ANARIArray2D radiance = anariNewArray2D(anari.device,hdri.pixels.data(),0,0,
                                                    ANARI_FLOAT32_VEC3,hdri.width,hdri.height);
            anariSetParameter(anari.device, al, "radiance", ANARI_ARRAY2D, &radiance);

            anariCommitParameters(anari.device, al);

            anariRelease(anari.device, radiance);

            anari.lights.push_back(al);
        }

        for (size_t l=0; l<lights.data.size(); ++l) {
            ANARILight al;
            auto &light = lights.data[l];
            osg::Light *osgLight = light.coLight.source->getLight();
            osg::NodePath np;
            np.push_back(light.coLight.root);
            np.push_back(light.coLight.source);
            osg::Vec4 pos = osgLight->getPosition();
            pos = pos * osg::Matrix::inverse(modelMat);
            if (pos.w() == 0.f) {
                al = anariNewLight(anari.device,"directional");
                anariSetParameter(anari.device, al, "direction",
                                  ANARI_FLOAT32_VEC3, pos.ptr());
            } else {
                al = anariNewLight(anari.device,"point");
                anariSetParameter(anari.device, al, "position",
                                  ANARI_FLOAT32_VEC3, pos.ptr());
            }

            anariCommitParameters(anari.device, al);

            anari.lights.push_back(al);
        }
        ANARIArray1D anariLights = anariNewArray1D(anari.device, anari.lights.data(), 0, 0,
                                                   ANARI_LIGHT, anari.lights.size());


        anariSetParameter(anari.device, anari.world, "light", ANARI_ARRAY1D, &anariLights);
        anariCommitParameters(anari.device, anari.world);

        anariRelease(anari.device, anariLights);

        lights.updated = false;
        hdri.updated = false;
    }

    // Even if the light _configuration_ hasn't change, the light properties
    // themselves might have

    for (size_t l=0; l<lights.data.size(); ++l) {

        osg::Light *oldLight = lights.data[l].coLight.source->getLight();
        osg::Light *newLight = newLights[l].coLight.source->getLight();

        osg::NodePath np;
        np.push_back(newLights[l].coLight.root);
        np.push_back(newLights[l].coLight.source);

        osg::Vec4 pos = newLights[l].coLight.source->getLight()->getPosition();
        pos = pos * osg::Matrix::inverse(modelMat);

        if (*oldLight != *newLight || lights.data[l].prevPos != pos) {
            lights.data[l].updated = true;
        }
    }

    // Update light properties:
    for (size_t l=0; l<lights.data.size(); ++l) {

        if (!lights.data[l].updated) continue;

        auto &light = lights.data[l].coLight;
        osg::Light *osgLight = light.source->getLight();
        osg::NodePath np;
        np.push_back(light.root);
        np.push_back(light.source);
        osg::Vec4 pos = osgLight->getPosition();
        pos = pos * osg::Matrix::inverse(modelMat);

        ANARILight al = anari.lights[l];
        if (pos.w() == 0.f) {
            anariSetParameter(anari.device, al, "direction",
                              ANARI_FLOAT32_VEC3, pos.ptr());
        } else {
            anariSetParameter(anari.device, al, "position",
                              ANARI_FLOAT32_VEC3, pos.ptr());
        }

        anariCommitParameters(anari.device, al);

        lights.data[l].prevPos = pos;
        lights.data[l].updated = false;
    }
}

void Renderer::setClipPlanes(const std::vector<Renderer::ClipPlane> &planes)
{
    bool doUpdate =  false;
    if (planes.size() != clipPlanes.data.size()) {
        doUpdate = true;
    } else {
        for (size_t i = 0; i < planes.size(); ++i) {
            for (int c = 0; c < 4; ++c) {
                if (clipPlanes.data[i][c] != planes[i][c]) {
                    doUpdate = true;
                    break;
                }

                if (doUpdate)
                    break;
            }
        }
    }

    if (doUpdate) {
        clipPlanes.data.clear();
        clipPlanes.data.insert(clipPlanes.data.begin(), planes.begin(), planes.end());
        clipPlanes.updated = true;
    }
}

void Renderer::setTransFunc(const glm::vec3 *rgb, unsigned numRGB,
                            const float *opacity, unsigned numOpacity)
{
    if (rgb && numRGB != 0) {
        transFunc.colors.resize(numRGB);
        memcpy(transFunc.colors.data(),
               rgb,
               transFunc.colors.size()*sizeof(transFunc.colors[0]));
    }

    if (opacity && numOpacity > 0) {
        transFunc.opacities.resize(numOpacity);
        memcpy(transFunc.opacities.data(),
               opacity,
               transFunc.opacities.size()*sizeof(transFunc.opacities[0]));
    }
    
    transFunc.updated = true;
    objectUpdates |= OU_TRANSFUNC_UPDATED;
}

void Renderer::setParam(std::string name, DataType type, std::any value)
{
    unsigned numParams = sizeof(params)/sizeof(params[0]);
    bool found=false;
    for (unsigned i=0; i<numParams; ++i) {
        if (params[i].name == name && params[i].type == type) {
            params[i].value = value;
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "no param: " << name << " of type " << toString(type) << std::endl;
        return;
    }

    objectUpdates |= OU_APP_PARAMS_UPDATED;

    // special handling for some params (TODO: deferred?!)
    if (name == "debug.colorByRank") {
        transFunc.updated = true;
    }
}

Param Renderer::getParam(std::string name)
{
    unsigned numParams = sizeof(params)/sizeof(params[0]);
    for (unsigned i=0; i<numParams; ++i) {
        if (params[i].name == name) return params[i];
    }

    return {};
}

void Renderer::wait()
{
#ifdef ANARI_PLUGIN_HAVE_MPI
    if (mpiSize > 1) {
        MPI_Barrier(MPI_COMM_WORLD);
    }
#endif
}

void Renderer::renderFrame()
{
    const bool isDisplayRank = mpiRank==displayRank;

    if (isDisplayRank && !multiChannelDrawer) {
#ifdef ANARI_PLUGIN_HAVE_CUDA
        multiChannelDrawer = new MultiChannelDrawer(false, anari.cudaInterop.enabled);
#else
        multiChannelDrawer = new MultiChannelDrawer(false, false);
#endif
        multiChannelDrawer->setMode(MultiChannelDrawer::AsIs);
        cover->getScene()->addChild(multiChannelDrawer);
    }

    if (anari.frames.empty())
        initFrames();

    if (anari.frames.empty()) // init failed!
        return;

    if (meshData.updated) {
        initMesh();
        meshData.updated = false;
    }

    // if (pointCloudData.fileNames.empty()) {
    //      pointCloudData.fileNames.push_back("random");
    //      pointCloudData.updated = true;
    // }
    if (pointCloudData.updated) {
        initPointClouds();
        pointCloudData.updated = false;
    }

    if (structuredVolumeData.updated) {
        initStructuredVolume();
        structuredVolumeData.updated = false;
    }

#ifdef HAVE_HDF5
    if (amrVolumeData.updated) {
        initAMRVolume();
        amrVolumeData.updated = false;
    }
#endif

    if (unstructuredVolumeData.updated) {
        initUnstructuredVolume();
        unstructuredVolumeData.updated = false;
    }

    if (transFunc.updated) {
        initTransFunc();
        transFunc.updated = false;
    }

    if (clipPlanes.updated) {
        initClipPlanes();
        clipPlanes.updated = false;
    }

    if (bounds.updated) {
        // set global bounds to local *before* we send them to peers!
        std::memcpy(&bounds.global.data[0],
                    &bounds.local.data[0],
                    sizeof(bounds.local.data));
        objectUpdates |= OU_BOUNDS_UPDATED;
        bounds.updated = false;
    }

    if (animation.updated) {
        initAnimation();
        animation.updated = false;
    }

    for (unsigned chan=0; chan<numChannels; ++chan) {
        renderFrame(chan);
    }

    wait();
}

void Renderer::renderFrame(unsigned chan)
{
    const bool isDisplayRank = mpiRank==displayRank;
    ChannelInfo &info = channelInfos[chan];

    int width=info.frame.width, height=info.frame.height;
    glm::mat4 mm=info.mm, mv=info.mv, pr=info.pr, vv{};
    if (isDisplayRank) {
        multiChannelDrawer->update();

        auto cam = coVRConfig::instance()->channels[chan].camera;
        auto vp = cam->getViewport();
        width = vp->width();
        height = vp->height();
    
        if (info.frame.width != width || info.frame.height != height) {
            objectUpdates |= OU_VIEWPORT_UPDATED;
        }

        mm = osg2glm(multiChannelDrawer->modelMatrix(chan));
        vv = osg2glm(multiChannelDrawer->viewMatrix(chan));
        mv = osg2glm(multiChannelDrawer->modelMatrix(chan) * multiChannelDrawer->viewMatrix(chan));
        pr = osg2glm(multiChannelDrawer->projectionMatrix(chan));

        if (info.mv != mv || info.pr != pr) {
            objectUpdates |= OU_CAMERA_UPDATED;
        }
    }

    // Remote rendering: client -> server
#ifdef ANARI_PLUGIN_HAVE_RR
    if (isClient) {
        // Exchange what updated:
        uint64_t myObjectUpdates = objectUpdates;
        rr->sendObjectUpdates(objectUpdates);

        if (myObjectUpdates & OU_VIEWPORT_UPDATED)
        {
            minirr::Viewport rrViewport;
            rrViewport.width = width;
            rrViewport.height = height;
            rr->sendViewport(rrViewport);
        }

        if (myObjectUpdates & OU_CAMERA_UPDATED)
        {
            minirr::Camera rrCamera;
            std::memcpy(rrCamera.modelMatrix, &mm[0], sizeof(rrCamera.modelMatrix));
            std::memcpy(rrCamera.viewMatrix, &vv[0], sizeof(rrCamera.viewMatrix));
            std::memcpy(rrCamera.projMatrix, &pr[0], sizeof(rrCamera.projMatrix));
            rr->sendCamera(rrCamera);
        }

        if (myObjectUpdates & OU_ANIMATION_UPDATED)
        {
            // client updates time step, server sets num time steps
            rr->sendAnimation(animation.timeStep, animation.numTimeSteps);
            int ignore=0;
            rr->recvAnimation(ignore, animation.numTimeSteps);
            animation.updated = true;
        }

        if (myObjectUpdates & OU_APP_PARAMS_UPDATED)
        {
            constexpr unsigned numParams = sizeof(params)/sizeof(params[0]);
            minirr::Param rrParams[numParams];

            typedef std::vector<uint8_t> ByteArray;
            std::vector<ByteArray> byteArrays(numParams);

            for (unsigned i=0; i<numParams; ++i) {
                rrParams[i].name = params[i].name;
                rrParams[i].type = (int)params[i].type;
                size_t len = sizeInBytes(params[i].type);
                byteArrays[i].resize(len);
                params[i].serialize(byteArrays[i].data());
                rrParams[i].value = byteArrays[i].data();
                rrParams[i].sizeInBytes = len;
            }

            rr->sendAppParams(rrParams, numParams);
        }
    
        if (myObjectUpdates & OU_TRANSFUNC_UPDATED)
        {
            minirr::Transfunc rrTransfunc;
            rrTransfunc.rgb = (float *)transFunc.colors.data();
            rrTransfunc.alpha = (float *)transFunc.opacities.data();
            rrTransfunc.numRGB = transFunc.colors.size();
            rrTransfunc.numAlpha = transFunc.opacities.size();
            rr->sendTransfunc(rrTransfunc);
            // TODO: ranges, scale
        }
    }

    if (isServer) {
        // Exchange what updated:
        uint64_t peerObjectUpdates{0x0};
        rr->recvObjectUpdates(peerObjectUpdates);

        if (peerObjectUpdates & OU_VIEWPORT_UPDATED)
        {
            minirr::Viewport rrViewport;
            rr->recvViewport(rrViewport);
            width = rrViewport.width;
            height = rrViewport.height;
        }

        if (peerObjectUpdates & OU_CAMERA_UPDATED)
        {
            minirr::Camera rrCamera;
            rr->recvCamera(rrCamera);
            std::memcpy(&mm[0], rrCamera.modelMatrix, sizeof(rrCamera.modelMatrix));
            std::memcpy(&vv[0], rrCamera.viewMatrix, sizeof(rrCamera.viewMatrix));
            std::memcpy(&pr[0], rrCamera.projMatrix, sizeof(rrCamera.projMatrix));
            mv = vv*mm;
        }

        if (peerObjectUpdates & OU_ANIMATION_UPDATED)
        {
            // client updates time step, server sets num time steps
            rr->sendAnimation(animation.timeStep, animation.numTimeSteps);
            int ignore=0;
            rr->recvAnimation(animation.timeStep, ignore);
            animation.updated = true;
        }

        if (peerObjectUpdates & OU_APP_PARAMS_UPDATED)
        {
            constexpr unsigned numParams = sizeof(params)/sizeof(params[0]);
            minirr::Param rrParams[numParams];
            unsigned numParamsReceived{0};
            rr->recvAppParams(rrParams, numParamsReceived);
            assert(numParamsReceived==numParams);

            for (unsigned i=0; i<numParams; ++i) {
                Param param;
                param.name = rrParams[i].name;
                param.type = (DataType)rrParams[i].type;
                param.unserialize(rrParams[i].value);
                setParam(param.name, param.type, param.value);
            }
        }

        if (peerObjectUpdates & OU_TRANSFUNC_UPDATED)
        {
            minirr::Transfunc rrTransfunc;
            rr->recvTransfunc(rrTransfunc);
            transFunc.colors.resize(rrTransfunc.numRGB);
            std::memcpy((float *)transFunc.colors.data(), rrTransfunc.rgb,
                sizeof(transFunc.colors[0])*transFunc.colors.size());
            transFunc.opacities.resize(rrTransfunc.numAlpha);
            std::memcpy((float *)transFunc.opacities.data(), rrTransfunc.alpha,
                sizeof(transFunc.opacities[0])*transFunc.opacities.size());
            // TODO: ranges, scale

            // consume on next renderFrame:
            transFunc.updated = true;
        }

        // Bump into *our* object updates; as what is now the server
        // is also possibly the main node of the cluster
        objectUpdates |= peerObjectUpdates;
    }
#endif

    // MPI comm
#ifdef ANARI_PLUGIN_HAVE_MPI
    if (mpiSize > 1) {
        uint64_t clusterObjectUpdates{0x0};
        if (mpiRank == mainRank) clusterObjectUpdates = objectUpdates;
        MPI_Bcast(&clusterObjectUpdates, 1, MPI_UINT64_T, mainRank, MPI_COMM_WORLD);

        if (clusterObjectUpdates & OU_VIEWPORT_UPDATED)
        {
            MPI_Bcast(&width, 1, MPI_INT, mainRank, MPI_COMM_WORLD);
            MPI_Bcast(&height, 1, MPI_INT, mainRank, MPI_COMM_WORLD);
        }

        if (clusterObjectUpdates & OU_CAMERA_UPDATED)
        {
            MPI_Bcast(&mm, sizeof(mm), MPI_BYTE, mainRank, MPI_COMM_WORLD);
            MPI_Bcast(&mv, sizeof(mv), MPI_BYTE, mainRank, MPI_COMM_WORLD);
            MPI_Bcast(&pr, sizeof(pr), MPI_BYTE, mainRank, MPI_COMM_WORLD);
        }

        if (clusterObjectUpdates & OU_BOUNDS_UPDATED)
        {
            MPI_Allreduce(&bounds.local.data[0], &bounds.global.data[0], 1, MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD);
            MPI_Allreduce(&bounds.local.data[1], &bounds.global.data[1], 1, MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD);
            MPI_Allreduce(&bounds.local.data[2], &bounds.global.data[2], 1, MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD);
            MPI_Allreduce(&bounds.local.data[3], &bounds.global.data[3], 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
            MPI_Allreduce(&bounds.local.data[4], &bounds.global.data[4], 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
            MPI_Allreduce(&bounds.local.data[5], &bounds.global.data[5], 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
        }

        if (clusterObjectUpdates & OU_ANIMATION_UPDATED)
        {
            MPI_Bcast(&animation.timeStep, 1, MPI_INT, mainRank, MPI_COMM_WORLD);
            MPI_Bcast(&animation.numTimeSteps, 1, MPI_INT, mainRank, MPI_COMM_WORLD);
            animation.updated = true;
        }

        if (clusterObjectUpdates & OU_APP_PARAMS_UPDATED)
        {
            unsigned numParams = sizeof(params)/sizeof(params[0]);

            typedef std::vector<uint8_t> ByteArray;
            std::vector<ByteArray> byteArrays(numParams);

            MPI_Bcast(&numParams, 1, MPI_UINT32_T, mainRank, MPI_COMM_WORLD);
            for (unsigned i=0; i<numParams; ++i) {
                size_t len = sizeInBytes(params[i].type);
                byteArrays[i].resize(len);
                params[i].serialize(byteArrays[i].data());
                MPI_Bcast(byteArrays[i].data(), byteArrays[i].size(), MPI_BYTE, mainRank, MPI_COMM_WORLD);

                Param param;
                param.name = params[i].name;
                param.type = params[i].type;
                param.unserialize(byteArrays[i].data());
                setParam(param.name, param.type, param.value);
            }
        }

        if (clusterObjectUpdates & OU_TRANSFUNC_UPDATED)
        {
            if (!getParam("debug.colorByRank").as<bool>()) { // debug colors -> each rank has its own TF!
                uint32_t numColors = transFunc.colors.size();
                MPI_Bcast(&numColors, 1, MPI_UINT32_T, mainRank, MPI_COMM_WORLD);
                transFunc.colors.resize(numColors);
                MPI_Bcast(transFunc.colors.data(), sizeof(transFunc.colors[0])*transFunc.colors.size(),
                    MPI_BYTE, mainRank, MPI_COMM_WORLD);

                uint32_t numOpacities = transFunc.opacities.size();
                MPI_Bcast(&numOpacities, 1, MPI_UINT32_T, mainRank, MPI_COMM_WORLD);
                transFunc.opacities.resize(numOpacities);
                MPI_Bcast(transFunc.opacities.data(), sizeof(transFunc.opacities[0])*transFunc.opacities.size(),
                    MPI_BYTE, mainRank, MPI_COMM_WORLD);

                // TODO: ranges, scale

                // consume on next renderFrame:
                transFunc.updated = true;
            }
        }
    } else
#endif

    // Remote rendering: server -> client
#ifdef ANARI_PLUGIN_HAVE_RR
    if (isClient) {
        // Exchange what updated:
        uint64_t peerObjectUpdates{0x0};
        rr->recvObjectUpdates(peerObjectUpdates);

        if (peerObjectUpdates & OU_BOUNDS_UPDATED)
        {
            minirr::AABB rrBounds;
            rr->recvBounds(rrBounds);
            std::memcpy(&bounds.global.data[0], &rrBounds[0], sizeof(rrBounds));
        }
    }
  
    if (isServer) {
        // Exchange what updated:
        uint64_t myObjectUpdates = objectUpdates;
        rr->sendObjectUpdates(objectUpdates);

        if (myObjectUpdates & OU_BOUNDS_UPDATED)
        {
            minirr::AABB rrBounds;
            std::memcpy(&rrBounds[0], &bounds.global.data[0], sizeof(rrBounds));
            rr->sendBounds(rrBounds);
        }
    }
#endif

    objectUpdates = 0x0;

    if (info.frame.width != width || info.frame.height != height) {
        info.frame.width = width;
        info.frame.height = height;
        info.frame.resized = true;

        unsigned imgSize[] = {(unsigned)width,(unsigned)height};
        anariSetParameter(anari.device, anari.frames[chan], "size", ANARI_UINT32_VEC2, imgSize);
        anariCommitParameters(anari.device, anari.frames[chan]);
    }

    if (info.mv != mv || info.pr != pr) {
        info.mv = mv;
        info.pr = pr;

        offaxisStereoCameraFromTransform(
            inverse(pr), inverse(mv), info.eye, info.dir, info.up, info.fovy, info.aspect, info.imgRegion);

        anariSetParameter(anari.device, anari.cameras[chan], "fovy", ANARI_FLOAT32, &info.fovy);
        anariSetParameter(anari.device, anari.cameras[chan], "aspect", ANARI_FLOAT32, &info.aspect);
        anariSetParameter(anari.device, anari.cameras[chan], "position", ANARI_FLOAT32_VEC3, &info.eye.x);
        anariSetParameter(anari.device, anari.cameras[chan], "direction", ANARI_FLOAT32_VEC3, &info.dir.x);
        anariSetParameter(anari.device, anari.cameras[chan], "up", ANARI_FLOAT32_VEC3, &info.up.x);
        anariSetParameter(anari.device, anari.cameras[chan], "imageRegion", ANARI_FLOAT32_BOX2, &info.imgRegion.min);
        anariCommitParameters(anari.device, anari.cameras[chan]);
    }

    if (info.mm != mm) {
        info.mm = mm;
        updateLights(glm2osg(mm));
    }

    #ifdef TIMING
    double t0 = getCurrentTime();
    #endif
    anariRenderFrame(anari.device, anari.frames[chan]);
    anariFrameReady(anari.device, anari.frames[chan], ANARI_WAIT);
    #ifdef TIMING
    double t1 = getCurrentTime();
    std::cout << "renderFrame() takes " << (t1-t0) << " sec.\n";
    #endif

#ifdef ANARI_PLUGIN_HAVE_RR
    if (isClient) {
        imageBuffer.resize(info.frame.width*info.frame.height);
        rr->recvImage(imageBuffer.data(), info.frame.width, info.frame.height,
                      getParam("rr.jpegQuality").as<int32_t>());
    }

    if (isServer) {
        uint32_t widthOUT;
        uint32_t heightOUT;
        ANARIDataType typeOUT;
        const uint32_t *fbPointer = (const uint32_t *)anariMapFrame(anari.device, anari.frames[chan],
                                                                    "channel.color",
                                                                    &widthOUT,
                                                                    &heightOUT,
                                                                    &typeOUT);
        rr->sendImage(fbPointer, widthOUT, heightOUT,
                      getParam("rr.jpegQuality").as<int32_t>());
    }
#endif

    // trigger redraw:
    if (isDisplayRank) {
        info.frame.updated = true;
    }
}

void Renderer::drawFrame()
{
    for (unsigned chan=0; chan<numChannels; ++chan) {
        drawFrame(chan);
    }
}

void Renderer::drawFrame(unsigned chan)
{
    const bool isDisplayRank = mpiRank==displayRank;
    if (!isDisplayRank)
        return;

    ChannelInfo &info = channelInfos[chan];
    if (info.frame.resized) {
        multiChannelDrawer->resizeView(
            chan, info.frame.width, info.frame.height, info.frame.colorFormat, info.frame.depthFormat);
        multiChannelDrawer->clearColor(chan);
        multiChannelDrawer->clearDepth(chan);
        info.frame.resized = false;
    }

    if (!info.frame.updated)
        return;

    if (isClient) {
        memcpy((uint32_t *)multiChannelDrawer->rgba(chan), imageBuffer.data(),
               sizeof(imageBuffer[0]) * imageBuffer.size());
#ifdef ANARI_PLUGIN_HAVE_CUDA
    } else if (anari.cudaInterop.enabled) {
        uint32_t widthOUT;
        uint32_t heightOUT;
        ANARIDataType typeOUT;
        const uint32_t *fbPointer = (const uint32_t *)anariMapFrame(anari.device, anari.frames[chan],
                                                                    "channel.colorGPU",
                                                                    &widthOUT,
                                                                    &heightOUT,
                                                                    &typeOUT);
        CUDA_SAFE_CALL(cudaMemcpyAsync((uint32_t *)multiChannelDrawer->rgba(chan), fbPointer,
               widthOUT*heightOUT*anari::sizeOf(typeOUT), cudaMemcpyDeviceToDevice,
               anari.cudaInterop.copyStream));
        CUDA_SAFE_CALL(cudaStreamSynchronize(anari.cudaInterop.copyStream));
        anariUnmapFrame(anari.device, anari.frames[chan], "channel.colorGPU");

        const float *dbPointer = (const float *)anariMapFrame(anari.device, anari.frames[chan],
                                                              "channel.depthGPU",
                                                              &widthOUT,
                                                              &heightOUT,
                                                              &typeOUT);
        float *dbXformed;
        CUDA_SAFE_CALL(cudaMalloc(&dbXformed, widthOUT*heightOUT*sizeof(float)));
        transformDepthFromWorldToGL_CUDA(dbPointer, dbXformed, info.eye, info.dir, info.up, info.fovy,
                                    info.aspect, info.imgRegion, info.mv, info.pr, widthOUT, heightOUT);
        CUDA_SAFE_CALL(cudaMemcpyAsync((float *)multiChannelDrawer->depth(chan), dbXformed,
               widthOUT*heightOUT*anari::sizeOf(typeOUT), cudaMemcpyDeviceToDevice,
               anari.cudaInterop.copyStream));
        CUDA_SAFE_CALL(cudaStreamSynchronize(anari.cudaInterop.copyStream));
        CUDA_SAFE_CALL(cudaFree(dbXformed));

        anariUnmapFrame(anari.device, anari.frames[chan], "channel.depthGPU");
#endif
    } else {
        uint32_t widthOUT;
        uint32_t heightOUT;
        ANARIDataType typeOUT;
        const uint32_t *fbPointer = (const uint32_t *)anariMapFrame(anari.device, anari.frames[chan],
                                                                    "channel.color",
                                                                    &widthOUT,
                                                                    &heightOUT,
                                                                    &typeOUT);
        memcpy((uint32_t *)multiChannelDrawer->rgba(chan), fbPointer,
               widthOUT*heightOUT*anari::sizeOf(typeOUT));
        anariUnmapFrame(anari.device, anari.frames[chan], "channel.color");

        const float *dbPointer = (const float *)anariMapFrame(anari.device, anari.frames[chan],
                                                              "channel.depth",
                                                              &widthOUT,
                                                              &heightOUT,
                                                              &typeOUT);
        std::vector<float> dbXformed(widthOUT*heightOUT);
        transformDepthFromWorldToGL(dbPointer, dbXformed.data(), info.eye, info.dir, info.up, info.fovy,
                                    info.aspect, info.imgRegion, info.mv, info.pr, widthOUT, heightOUT);
        memcpy((float *)multiChannelDrawer->depth(chan), dbXformed.data(),
               widthOUT*heightOUT*anari::sizeOf(typeOUT));

        anariUnmapFrame(anari.device, anari.frames[chan], "channel.depth");
    }

    multiChannelDrawer->swapFrame();

    // frame was consumed:
    info.frame.updated = false;
}

void Renderer::initMPI()
{
#ifdef ANARI_PLUGIN_HAVE_MPI
    int mpiInitCalled = 0;
    MPI_Initialized(&mpiInitCalled);

    if (mpiInitCalled) {
        MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    }

    // can be set to a value outside of the range in
    // the config for headless clusters:
    bool displayRankEntryExists = false;
    displayRank  = covise::coCoviseConfig::getInt(
        "displayRank",
        "COVER.Plugin.ANARI.Cluster",
        0,
        &displayRankEntryExists
    );
#endif
}

void Renderer::initRR()
{
#ifdef ANARI_PLUGIN_HAVE_RR
    bool modeEntryExists = false;
    std::string mode = covise::coCoviseConfig::getEntry(
        "mode",
        "COVER.Plugin.ANARI.RR",
        "",
        &modeEntryExists
    );

    bool hostnameEntryExists = false;
    std::string hostname = covise::coCoviseConfig::getEntry(
        "hostname",
        "COVER.Plugin.ANARI.RR",
        "localhost",
        &hostnameEntryExists
    );

    bool portEntryExists = false;
    unsigned short port  = covise::coCoviseConfig::getInt(
        "port",
        "COVER.Plugin.ANARI.RR",
        31050,
        &portEntryExists
    );

    // TODO:
    isServer = mpiRank == mainRank && modeEntryExists && mode == "server";
    isClient = modeEntryExists && mode == "client";

    rr = std::make_shared<minirr::MiniRR>();

    if (isClient) {
        std::cout << "ANARI.RR.mode is 'client'\n";
        rr->initAsClient(hostname, port);
        rr->run();
    }

    if (isServer) {
        std::cout << "ANARI.RR.mode is 'server'\n";
        rr->initAsServer(port);
        rr->run();
    }
#endif
}

void Renderer::initChannels()
{
    const bool isDisplayRank = mpiRank==displayRank;

    if (isDisplayRank) {
        numChannels = coVRConfig::instance()->numChannels();
    }

#ifdef ANARI_PLUGIN_HAVE_RR
    if (isClient) {
        rr->sendNumChannels(numChannels);
    }

    if (isServer) {
        rr->recvNumChannels(numChannels);
    }
#endif

#ifdef ANARI_PLUGIN_HAVE_MPI
    if (mpiSize > 1) {
        MPI_Bcast(&numChannels, 1, MPI_INT, mainRank, MPI_COMM_WORLD);
    }
#endif

    channelInfos.resize(numChannels);
}

void Renderer::initDevice()
{
    bool libraryEntryExists = false;
    anari.libtype = covise::coCoviseConfig::getEntry(
        "value",
        "COVER.Plugin.ANARI.Library",
        "environment",
        &libraryEntryExists
    );

    bool hostnameEntryExists = false;
    std::string hostname = covise::coCoviseConfig::getEntry(
        "hostname",
        "COVER.Plugin.ANARI.RemoteServer",
        "localhost",
        &hostnameEntryExists
    );

    bool portEntryExists = false;
    unsigned short port = covise::coCoviseConfig::getInt(
        "port",
        "COVER.Plugin.ANARI.RemoteServer",
        31050,
        &portEntryExists
    );

    anari.library = anariLoadLibrary(anari.libtype.c_str(), statusFunc);
    if (!anari.library) return;
    anari.device = anariNewDevice(anari.library, anari.devtype.c_str());
    if (!anari.device) return;
    if (anari.libtype == "remote") {
        if (hostnameEntryExists)
            anariSetParameter(anari.device, anari.device, "server.hostname", ANARI_STRING,
                              hostname.c_str());

        if (portEntryExists)
            anariSetParameter(anari.device, anari.device, "server.port", ANARI_UINT16, &port);
    }
    anariCommitParameters(anari.device, anari.device);
    anari.world = anari::newObject<anari::World>(anari.device);
#ifdef ANARI_PLUGIN_HAVE_CUDA
    anari.cudaInterop.enabled
        = deviceHasExtension(anari.library, "default", "ANARI_VISRTX_CUDA_OUTPUT_BUFFERS");
    if (anari.cudaInterop.enabled) {
        CUDA_SAFE_CALL(cudaStreamCreate(&anari.cudaInterop.copyStream));
    }
#endif
}

void Renderer::initFrames()
{
    anari.renderer = anariNewRenderer(anari.device, anari.renderertype.c_str());

    float r = coCoviseConfig::getFloat("r", "COVER.Background", 0.0f);
    float g = coCoviseConfig::getFloat("g", "COVER.Background", 0.0f);
    float b = coCoviseConfig::getFloat("b", "COVER.Background", 0.0f);
    float bgcolor[] = {r,g,b,1.f};

    anariSetParameter(anari.device, anari.renderer, "background", ANARI_FLOAT32_VEC4,
                      bgcolor);
    anariCommitParameters(anari.device, anari.renderer);

    anari.frames.resize(numChannels);
    anari.cameras.resize(numChannels);
    for (unsigned chan=0; chan<numChannels; ++chan) {
        ANARIFrame &frame = anari.frames[chan];
        ANARICamera &camera = anari.cameras[chan];

        frame = anariNewFrame(anari.device);
        anariSetParameter(anari.device, frame, "world", ANARI_WORLD, &anari.world);

        ANARIDataType fbFormat = ANARI_UFIXED8_RGBA_SRGB;
        ANARIDataType dbFormat = ANARI_FLOAT32;
        anariSetParameter(anari.device, frame, "channel.color", ANARI_DATA_TYPE, &fbFormat);
        anariSetParameter(anari.device, frame, "channel.depth", ANARI_DATA_TYPE, &dbFormat);
        anariSetParameter(anari.device, frame, "renderer", ANARI_RENDERER, &anari.renderer);

        camera = anariNewCamera(anari.device, "perspective");
        anariSetParameter(anari.device, frame, "camera", ANARI_CAMERA, &camera);
        anariCommitParameters(anari.device, frame);
    }
}

// Scene loading

#define ASG_SAFE_CALL(X) X // TODO!

void Renderer::initMesh()
{
    const char *fileName = meshData.fileName.c_str();

    if (!anari.root)
        anari.root = asgNewObject();

    anari.meshes = asgNewObject();

    // Load from file
    std::string ext = getExt(fileName);
    if (ext==".pbf" || ext==".pbrt")
        ASG_SAFE_CALL(asgLoadPBRT(anari.meshes, fileName, 0));
    else
        ASG_SAFE_CALL(asgLoadASSIMP(anari.meshes, fileName, 0));

    ASG_SAFE_CALL(asgObjectAddChild(anari.root, anari.meshes));

    // Build up ANARI world
    ASGBuildWorldFlags_t flags = ASG_BUILD_WORLD_FLAG_GEOMETRIES |
                                 ASG_BUILD_WORLD_FLAG_MATERIALS  |
                                 ASG_BUILD_WORLD_FLAG_TRANSFORMS;
    ASG_SAFE_CALL(asgBuildANARIWorld(anari.root, anari.device, anari.world, flags, 0));

    anariCommitParameters(anari.device, anari.world);

    AABB bounds;
    asgComputeBounds(anari.meshes,
                     &bounds.data[0],&bounds.data[1],&bounds.data[2],
                     &bounds.data[3],&bounds.data[4],&bounds.data[5]);
    this->bounds.local.extend(bounds);
    this->bounds.updated = true;
}

void Renderer::initPointClouds()
{
    auto surfaceArray
        = anari::newArray1D(anari.device, ANARI_SURFACE, pointCloudData.fileNames.size());

    bool radiusEntryExists = false;
    float radius  = covise::coCoviseConfig::getFloat(
        "radius",
        "COVER.Plugin.ANARI.PointCloud",
        0.1f,
        &radiusEntryExists
    );

    auto *s = anari::map<anari::Surface>(anari.device, surfaceArray);
    for (size_t i = 0; i < pointCloudData.fileNames.size(); ++i) {
        std::string fn(pointCloudData.fileNames[i]);
        // TODO: import
        if (fn == "random") {
            auto surface = generateRandomSpheres(anari.device, glm::vec3(0.f));
            s[i] = surface;
            anariRelease(anari.device, surface);
        } else if (getExt(fn)==".pts") {
            auto surface = readPTS(anari.device, fn, radius);
            s[i] = surface;
	        anariRelease(anari.device, surface);
	    } else if (getExt(fn)==".ply") {
            auto surface = readPLY(anari.device, fn, radius);
            s[i] = surface;
	        anariRelease(anari.device, surface);
	    }
    }
    anari::unmap(anari.device, surfaceArray);
    anari::setAndReleaseParameter(anari.device, anari.world, "surface", surfaceArray);

    anariCommitParameters(anari.device, anari.world);

    AABB bounds;
    anariGetProperty(
        anari.device, anari.world, "bounds", ANARI_FLOAT32_BOX3, &bounds.data, sizeof(bounds.data), ANARI_WAIT);
    this->bounds.local.extend(bounds);
    this->bounds.updated = true;
}

void Renderer::initStructuredVolume()
{
    if (structuredVolumeData.bpc == 1) {
        // Convert to float..
        structuredVolumeData.voxels.resize(
            structuredVolumeData.sizeX*size_t(structuredVolumeData.sizeY)*structuredVolumeData.sizeZ);

        for (size_t i=0; i<structuredVolumeData.voxels.size(); ++i) {
            structuredVolumeData.voxels[i] = ((const uint8_t *)structuredVolumeData.data)[i]/255.999f;
        }

        if (!anari.root)
            anari.root = asgNewObject();

        anari.structuredVolume = asgNewStructuredVolume(structuredVolumeData.voxels.data(),
                                                        structuredVolumeData.sizeX,
                                                        structuredVolumeData.sizeY,
                                                        structuredVolumeData.sizeZ,
                                                        ASG_DATA_TYPE_FLOAT32, nullptr);
        ASG_SAFE_CALL(asgStructuredVolumeSetRange(anari.structuredVolume,
                                                  structuredVolumeData.minValue,
                                                  structuredVolumeData.maxValue));

        structuredVolumeData.rgbLUT.resize(15);
        structuredVolumeData.alphaLUT.resize(5);

        initTransFunc();

        ASG_SAFE_CALL(asgObjectAddChild(anari.root, anari.structuredVolume));

        ASGBuildWorldFlags_t flags = ASG_BUILD_WORLD_FLAG_VOLUMES |
                                     ASG_BUILD_WORLD_FLAG_LUTS;
        ASG_SAFE_CALL(asgBuildANARIWorld(anari.root, anari.device, anari.world, flags, 0));

        anariCommitParameters(anari.device, anari.world);

        // asgComputeBounds doesn't work for volumes yet...
        AABB bounds;
        bounds.data[0] = 0.f;
        bounds.data[1] = 0.f;
        bounds.data[2] = 0.f;
        bounds.data[3] = structuredVolumeData.sizeX;
        bounds.data[4] = structuredVolumeData.sizeY;
        bounds.data[5] = structuredVolumeData.sizeZ;
        this->bounds.local.extend(bounds);
        this->bounds.updated = true;
    }

    if (structuredVolumeData.deleteData) {
        switch (structuredVolumeData.bpc) {
            case 1:
                delete[] (uint8_t *)structuredVolumeData.data;
                break;

            case 2:
                delete[] (uint16_t *)structuredVolumeData.data;
                break;

            case 4:
                delete[] (float *)structuredVolumeData.data;
                break;
        }
        structuredVolumeData.deleteData = false;
    }
}

void Renderer::initAMRVolume()
{
#ifdef HAVE_HDF5
    anari.amrVolume.field = anariNewSpatialField(anari.device, "amr");
    // TODO: "amr" field is an extension - check if it is supported!
    auto &data = amrVolumeData.data;
    std::vector<anari::Array3D> blockDataV(data.blockData.size());
    for (size_t i = 0; i < data.blockData.size(); ++i) {
        blockDataV[i] = anari::newArray3D(anari.device, data.blockData[i].values.data(),
                                          data.blockData[i].dims[0],
                                          data.blockData[i].dims[1],
                                          data.blockData[i].dims[2]);
    }
    printf("Array sizes:\n");
    printf("    'cellWidth'  : %zu\n", data.cellWidth.size());
    printf("    'blockBounds': %zu\n", data.blockBounds.size());
    printf("    'blockLevel' : %zu\n", data.blockLevel.size());
    printf("    'blockData'  : %zu\n", blockDataV.size());

    anari::setParameterArray1D(anari.device, anari.amrVolume.field, "cellWidth", ANARI_FLOAT32,
                               data.cellWidth.data(), data.cellWidth.size());
    anari::setParameterArray1D(anari.device, anari.amrVolume.field, "block.bounds", ANARI_INT32_BOX3,
                               data.blockBounds.data(),
                               data.blockBounds.size());
    anari::setParameterArray1D(anari.device, anari.amrVolume.field, "block.level", ANARI_INT32,
                               data.blockLevel.data(), data.blockLevel.size());
    anari::setParameterArray1D(anari.device, anari.amrVolume.field, "block.data", ANARI_ARRAY1D,
                               blockDataV.data(), blockDataV.size());

    for (auto a : blockDataV)
        anari::release(anari.device, a);

    anariCommitParameters(anari.device, anari.amrVolume.field);

    amrVolumeData.minValue = data.voxelRange.x;
    amrVolumeData.maxValue = data.voxelRange.y;

    anari.amrVolume.volume = anari::newObject<anari::Volume>(anari.device, "transferFunction1D");
    anari::setParameter(anari.device, anari.amrVolume.volume, "value", anari.amrVolume.field);

    initTransFunc();

    anariSetParameter(anari.device, anari.amrVolume.volume, "valueRange", ANARI_FLOAT32_BOX1,
                      &data.voxelRange);

    anari::commitParameters(anari.device, anari.amrVolume.volume);

    anari::setAndReleaseParameter(anari.device, anari.world, "volume",
                                  anari::newArray1D(anari.device, &anari.amrVolume.volume));
    anariRelease(anari.device, anari.amrVolume.volume);
    anariCommitParameters(anari.device, anari.world);

    AABB bounds;
    anariGetProperty(
        anari.device, anari.world, "bounds", ANARI_FLOAT32_BOX3, &bounds.data, sizeof(bounds.data), ANARI_WAIT);
    this->bounds.local.extend(bounds);
    this->bounds.updated = true;
#endif
}

void Renderer::initUnstructuredVolume()
{
    UnstructuredField data;
    int numTimeSteps = 1;

    if (unstructuredVolumeData.readerType == UMESH) {
#ifdef HAVE_UMESH
        for (const auto &sf : unstructuredVolumeData.umeshScalarFiles) {
            unstructuredVolumeData.umeshReader.addFieldFromFile(
                sf.fileName.c_str(), sf.fieldID, sf.slotID, sf.timeStep);
        }

        // here we'll init the field in the loop (don't know if ts=0 is actually loaded..)
        numTimeSteps = unstructuredVolumeData.umeshReader.numTimeSteps();
#else
        return;
#endif
    }

    float dataRange[] = {1e30f,-1e30f};

    ANARISpatialField firstField{nullptr};

    for (int timeStep=0; timeStep<numTimeSteps; ++timeStep) {
        data = {}; //clear!
#ifdef HAVE_UMESH
        if (unstructuredVolumeData.readerType == UMESH
            && unstructuredVolumeData.umeshReader.haveTimeStep(timeStep))
            data = unstructuredVolumeData.umeshReader.getField(0,timeStep);
#endif

#ifdef HAVE_VTK
        if (unstructuredVolumeData.readerType == VTK) {
            assert(timeStep == 0);
            data = unstructuredVolumeData.vtkReader.getField(0);
        }
#endif

        if (unstructuredVolumeData.readerType == UNKNOWN) {
            assert(timeStep == 0);
            // comes from a data source!
            data = unstructuredVolumeData.dataSource;
        }

        if (data.vertexPosition.empty()) {
            continue;
        }

        // TODO: "unstructured" field is an extension - check if it is supported!
        auto field = anari::newObject<anari::SpatialField>(anari.device, "unstructured");
        printf("Array sizes:\n");
        printf("    'vertexPosition': %zu\n", data.vertexPosition.size());
        printf("    'vertexData'    : %zu\n", data.vertexData.size());
        printf("    'index'         : %zu\n", data.index.size());
        printf("    'cellIndex'     : %zu\n", data.cellIndex.size());

        anari::setParameterArray1D(anari.device, field, "vertex.position",
                ANARI_FLOAT32_VEC3, data.vertexPosition.data(), data.vertexPosition.size());
        anari::setParameterArray1D(anari.device, field, "vertex.data",
                ANARI_FLOAT32, data.vertexData.data(), data.vertexData.size());
        anari::setParameterArray1D(anari.device, field, "index",
                ANARI_UINT64, data.index.data(), data.index.size());
        anari::setParameter(anari.device, field, "indexPrefixed",
                            ANARI_BOOL, &data.indexPrefixed);
        anari::setParameterArray1D(anari.device, field, "cell.index",
                ANARI_UINT64, data.cellIndex.data(), data.cellIndex.size());
        anari::setParameterArray1D(anari.device, field, "cell.type",
                                   ANARI_UINT8, data.cellType.data(), data.cellType.size());

        anari::commitParameters(anari.device, field);

        anari.unstructuredVolume.fields.push_back(field);

        dataRange[0] = fminf(dataRange[0],data.dataRange.x);
        dataRange[1] = fmaxf(dataRange[1],data.dataRange.y);
    }

    if (!anari.unstructuredVolume.volume && !anari.unstructuredVolume.fields.empty()) {
        anari.unstructuredVolume.volume = anari::newObject<anari::Volume>(anari.device, "transferFunction1D");
        anari::setParameter(anari.device, anari.unstructuredVolume.volume, "id", (unsigned)mpiRank);

        initTransFunc();

        anari::setAndReleaseParameter(anari.device, anari.world, "volume",
                                      anari::newArray1D(anari.device, &anari.unstructuredVolume.volume));
        anariCommitParameters(anari.device, anari.world);

        anari::setParameter(anari.device, anari.unstructuredVolume.volume, "value",
                            anari.unstructuredVolume.fields[0]);

        anariSetParameter(anari.device, anari.unstructuredVolume.volume, "valueRange", ANARI_FLOAT32_BOX1,
                          dataRange);
        anari::commitParameters(anari.device, anari.unstructuredVolume.volume);

        AABB bounds;
        anariGetProperty(
            anari.device, anari.world, "bounds", ANARI_FLOAT32_BOX3, &bounds.data, sizeof(bounds.data), ANARI_WAIT);
        this->bounds.local.extend(bounds);
        this->bounds.updated = true;
    }

    animation.numTimeSteps = anari.unstructuredVolume.fields.size();
    animation.updated = true;
    objectUpdates |= OU_ANIMATION_UPDATED;
}

void Renderer::initClipPlanes()
{
    if (!anari.renderer)
        return;

    if (clipPlanes.data.empty()) {
        anari::unsetParameter(anari.device, anari.renderer, "clipPlane");
    } else {
        anari::setAndReleaseParameter(
            anari.device, anari.renderer, "clipPlane",
            anari::newArray1D(anari.device, clipPlanes.data.data(), clipPlanes.data.size()));
    }

    anari::commitParameters(anari.device, anari.renderer);
}

void Renderer::generateTransFunc()
{
    // dflt. color map:
    if (transFunc.colors.empty()) {
        transFunc.colors.emplace_back(0.f, 0.f, 1.f);
        transFunc.colors.emplace_back(0.f, 1.f, 0.f);
        transFunc.colors.emplace_back(1.f, 0.f, 0.f);
    }

    // dflt. alpha ramp:
    if (transFunc.opacities.empty()) {
        transFunc.opacities.emplace_back(0.f);
        transFunc.opacities.emplace_back(1.f);
    }
}

void Renderer::initTransFunc()
{
    ANARIVolume anariVolume{nullptr};
    if (anari.structuredVolume)
    {
        asgStructuredVolumeGetAnariHandle(anari.structuredVolume, &anariVolume);
    }

    if (anari.amrVolume.volume)
    {
        anariVolume = anari.amrVolume.volume;
    }

    if (anari.unstructuredVolume.volume)
    {
        anariVolume = anari.unstructuredVolume.volume;
    }

    std::vector<glm::vec3> colors;
    if (getParam("debug.colorByRank").as<bool>()) {
        auto c = randomColor(mpiRank);
        colors.emplace_back(c.x, c.y, c.z);
    } else {
        colors = transFunc.colors;
    }

    if (anariVolume) {
        anari::setAndReleaseParameter(
            anari.device, anariVolume, "color",
            anari::newArray1D(anari.device, colors.data(), colors.size()));
        anari::setAndReleaseParameter(
            anari.device, anariVolume, "opacity",
            anari::newArray1D(anari.device, transFunc.opacities.data(), transFunc.opacities.size()));
        anari::commitParameters(anari.device, anariVolume);
    }
}

void Renderer::initAnimation()
{
    // currently only used with umesh fields:
    if (animation.numTimeSteps <= animation.timeStep) {
        std::cerr << "time step unavailable: " << animation.timeStep << '\n';
        return;
    }

    // client e.g. has no field data
    if (animation.timeStep < anari.unstructuredVolume.fields.size()) {
        anari::setParameter(anari.device, anari.unstructuredVolume.volume, "value",
                            anari.unstructuredVolume.fields[animation.timeStep]);

        anari::commitParameters(anari.device, anari.unstructuredVolume.volume);
    }
}


