/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

/**************************************************************************\
 **                                                           (C)2013 RUS  **
 **                                                                        **
 ** Description: Read FOAM data format                                     **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** History:                                                               **
 ** May   13        C.Kopf          V1.0                                   **
 *\**************************************************************************/

#include "ReadFOAM.h"
#include <do/coDoUnstructuredGrid.h>
#include <do/coDoData.h>
#include <do/coDoSet.h>
#include <util/coFileUtil.h>
#include <util/coRestraint.h>

#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <cctype>
#include <limits>

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


ReadFOAM::ReadFOAM(int argc, char *argv[]) //Constructor
    : coModule(argc, argv, "Read OpenFOAM Data") // description in the module setup window
{
    //Set Number of Data Ports here. Default=3
    num_ports = 3;
    num_boundary_data_ports = 3;
    //Setup vectors for last used data
    lastDataPortSelection.resize(num_ports);
    lastBoundaryPortSelection.resize(num_boundary_data_ports);
    lastParticlePortSelection.resize(num_ports);

    // file browser parameter
    filenameParam = addFileBrowserParam("casedir", "Data file path");
    filenameParam->setValue("~", "*.*");

    //Scalar parameters
    starttimeParam = addFloatParam("starttime", "Start time");
    starttimeParam->setValue(0.0);
    stoptimeParam = addFloatParam("stoptime", "Stop time");
    stoptimeParam->setValue(1000000.0);
    skipfactorParam = addInt32Param("skipfactor", "1 means skip nothing, 2 means skip every 2nd time-directory ... ");
    skipfactorParam->setValue(1);

    //Mesh Boolean parameter (can be set to false to prevent the parsing of the grid)
    meshParam = addBooleanParam("load_unstructured_grid", "set to false to prevent loading of mesh");
    meshParam->setValue(true);

    //Choice parameters
    for (int i = 0; i < num_ports; ++i)
    {
        coChoiceParam *choice;
        std::stringstream s, ss;
        s << "DataChoice" << i;
        ss << "Choose data to map to DataPort " << i;
        choice = addChoiceParam(s.str().c_str(), ss.str().c_str());
        portChoice.push_back(choice);
    }

    //Boundary Boolean parameter (can be set to false to prevent the parsing of the boundary)
    boundaryParam = addBooleanParam("load_boundary_polygon", "set to false to prevent loading of boundary patches");
    boundaryParam->setValue(true);

    //Boundary Parts Parameter
    patchesStringParam = addStringParam("boundary_patches", "string to specify boundary patches");
    patchesStringParam->setValue("all");

    //Boundary Data choice parameters
    for (int i = 0; i < num_boundary_data_ports; ++i)
    {
        coChoiceParam *choice;
        std::stringstream s, ss;
        s << "BoundaryDataChoice" << i;
        ss << "Choose data to map to BoundaryDataPort " << i;
        choice = addChoiceParam(s.str().c_str(), ss.str().c_str());
        boundaryDataChoice.push_back(choice);
    }

    particleParam = addBooleanParam("load_particles", "set to false to prevent loading of particle data");
    particleParam->setValue(true);
    for (int i=0; i<num_ports; ++i)
    {
        std::stringstream s, ss;
        s << "ParticleDataChoice" << i;
        ss << "Choose data to map to ParticleDataPort " << i;
        coChoiceParam *choice = addChoiceParam(s.str().c_str(), ss.str().c_str());
        particleDataChoice.push_back(choice);
    }

    // the output ports
    meshOutPort = addOutputPort("GridOut0", "UnstructuredGrid", "unstructured grid");
    //Data output Ports
    for (int i = 0; i < num_ports; ++i)
    {
        coOutputPort *outPort;
        std::stringstream s, ss, sss;
        s << "DataOut" << i;
        ss << "Vec3|Float";
        sss << "Outputs data that has been set in Parameter: DataChoice" << i;
        outPort = addOutputPort(s.str().c_str(), ss.str().c_str(), sss.str().c_str());
        outPorts.push_back(outPort);
    }
    boundaryOutPort = addOutputPort("GridOut1", "Polygons", "Boundary Polygons");
    //Boundary data output ports
    for (int i = 0; i < num_ports; ++i)
    {
        coOutputPort *outPort;
        std::stringstream s, ss, sss;
        s << "BoundaryDataOut" << i;
        ss << "Vec3|Float";
        sss << "Outputs data that has been set in Parameter: BoundaryDataPort" << i;
        outPort = addOutputPort(s.str().c_str(), ss.str().c_str(), sss.str().c_str());
        boundaryDataPorts.push_back(outPort);
    }
    particleOutPort = addOutputPort("GridOut2", "Points", "Particles");
    //particle data output ports
    for (int i = 0; i < num_ports; ++i)
    {
        coOutputPort *outPort;
        std::stringstream s, ss, sss;
        s << "ParticleDataOut" << i;
        ss << "Vec3|Float|Int";
        sss << "Outputs data that has been set in Parameter: ParticleDataPort" << i;
        outPort = addOutputPort(s.str().c_str(), ss.str().c_str(), sss.str().c_str());
        particleDataPorts.push_back(outPort);
    }
}

ReadFOAM::~ReadFOAM() //Destructor
{
}

std::vector<const char *> ReadFOAM::getFieldList()
{
    int num = int(m_case.varyingFields.size() + m_case.constantFields.size() + 2);
    std::vector<const char *> choiceVal(num);
    int i = 0;
    choiceVal[i] = "none";
    ++i;
    choiceVal[i] = "processorID";
    ++i;


    for (std::map<std::string, int>::iterator it = m_case.varyingFields.begin();
         it != m_case.varyingFields.end();
         ++it)
    {
        choiceVal[i] = it->first.c_str();
        ++i;
    }

    for (std::map<std::string, int>::iterator it = m_case.constantFields.begin();
         it != m_case.constantFields.end();
         ++it)
    {
        choiceVal[i] = it->first.c_str();
        ++i;
    }

    return choiceVal;
}

std::vector<const char *> ReadFOAM::getParticleFieldList()
{
    int num = int(m_case.particleFields.size()+2);
    std::vector<const char *> choiceVal(num);
    int i = 0;
    choiceVal[i] = "none";
    ++i;
    choiceVal[i] = "cellID";
    ++i;

    for (std::map<std::string, int>::iterator it = m_case.particleFields.begin();
         it != m_case.particleFields.end();
         ++it)
    {
        choiceVal[i] = it->first.c_str();
        ++i;
    }

    return choiceVal;
}

void ReadFOAM::param(const char *paramName, bool inMapLoading)
{
    if (string(paramName) == "casedir")
    {
        // read the file browser parameter
        casedir = filenameParam->getValue();

        coModule::sendInfo("Checking Case-Directory. Please wait ...");
        int skipfactor = skipfactorParam->getValue();
        if (skipfactor < 1)
        {
            skipfactor = 1;
            skipfactorParam->setValue(1);
        }

        m_case = getCaseInfo(casedir);
        if (!m_case.valid)
        {
            coModule::sendError("%s is not a valid OpenFOAM case", casedir);
        }
        else
        {
            coModule::sendInfo("Done.");
            coModule::sendInfo("Case-directory: %s", casedir);
            coModule::sendInfo("Number of processors: %d", m_case.numblocks);
            coModule::sendInfo("Number of available time directories: %ld", (long)m_case.timedirs.size());
            std::stringstream timeinfo;
            timeinfo << "Available Time Directories: | ";
            for (std::map<double, std::string>::const_iterator it = m_case.timedirs.begin(); it != m_case.timedirs.end(); ++it)
            {
                timeinfo << it->second << " | ";
            }
            coModule::sendInfo("%s", timeinfo.str().c_str());
        }

        //Print Boundary-Patch List into covise
        std::stringstream meshdir;
        if (m_case.numblocks > 0)
        {
            meshdir << "processor0/constant/polyMesh"; //<< m_case.constantdir << "/polyMesh";
        }
        else
        {
            meshdir << "constant/polyMesh"; //<< m_case.constantdir << "/polyMesh";
        }
        coModule::sendInfo("%s", meshdir.str().c_str());
        coModule::sendInfo("Listing Boundary Patches!");
        Boundaries bounds = m_case.loadBoundary(meshdir.str());
        for (int i = 0; i < bounds.boundaries.size(); ++i)
        {
            std::stringstream info;
            info << bounds.boundaries[i].index << " ## " << bounds.boundaries[i].name;
            coModule::sendInfo("%s", info.str().c_str());
        }

        coModule::sendInfo("Listing varying Fields!");
        for (std::map<std::string, int>::iterator it = m_case.varyingFields.begin();
                it != m_case.varyingFields.end();
                ++it)
        {
            coModule::sendInfo("%s", it->first.c_str());
        }

        //fill the choiceParameters and set them to the previously selected item (if not possible set them to "none")
        {
            std::vector<const char *> choiceVal = getFieldList();
            index_t num = int(choiceVal.size());

            for (int i = 0; i < num_ports; ++i)
            {
                std::string lastSelection = lastDataPortSelection[i];
                int last = 0;
                for (int j = 0; j < choiceVal.size(); ++j)
                {
                    if (strcmp(lastSelection.c_str(), choiceVal[j]) == 0)
                    {
                        last = j;
                        break;
                    }
                }
                portChoice[i]->setValue(num, &choiceVal[0], last);
            }
            for (int i = 0; i < num_boundary_data_ports; ++i)
            {
                std::string lastSelection = lastBoundaryPortSelection[i];
                int last = 0;
                for (int j = 0; j < choiceVal.size(); ++j)
                {
                    if (strcmp(lastSelection.c_str(), choiceVal[j]) == 0)
                    {
                        last = j;
                        break;
                    }
                }
                boundaryDataChoice[i]->setValue(num, &choiceVal[0], last);
            }
        }

        {
            std::vector<const char *> choiceVal = getParticleFieldList();
            index_t num = int(choiceVal.size());
            for (int i=0; i<num_ports; ++i)
            {
                std::string lastSelection = lastParticlePortSelection[i];
                int last = 0;
                for (int j = 0; j < choiceVal.size(); ++j)
                {
                    if (strcmp(lastSelection.c_str(), choiceVal[j]) == 0)
                    {
                        last = j;
                        break;
                    }
                }
                particleDataChoice[i]->setValue(num, &choiceVal[0], last);
            }
        }
        std::cerr << "varyingcoords " << m_case.varyingCoords << " varyinggrid " << m_case.varyingGrid << std::endl;
    }

    //whenever a choice parameter get set -> save the string in vector lastDataPortSelection or lastBoundaryPortSelection
    for (int i = 0; i < num_ports; ++i)
    {
        std::stringstream s;
        s << "DataChoice" << i;
        if (string(paramName) == s.str())
        {
            index_t portIndex = portChoice[i]->getValue();
            std::string portFilename = portChoice[i]->getLabel(portIndex);
            lastDataPortSelection[i] = portFilename;
        }
    }
    for (int i = 0; i < num_boundary_data_ports; ++i)
    {
        std::stringstream s;
        s << "BoundaryDataChoice" << i;
        if (string(paramName) == s.str())
        {
            index_t portIndex = boundaryDataChoice[i]->getValue();
            std::string portFilename = boundaryDataChoice[i]->getLabel(portIndex);
            lastBoundaryPortSelection[i] = portFilename;
        }
    }
    for (int i=0; i<num_ports; ++i)
    {
        std::stringstream s;
        s << "ParticleDataChoice" << i;
        if (string(paramName) == s.str())
        {
            index_t portIndex = particleDataChoice[i]->getValue();
            std::string portFilename = particleDataChoice[i]->getLabel(portIndex);
            lastBoundaryPortSelection[i] = portFilename;
        }
    }

    //While opening a .net file -> If all choiceParameters have been loaded -> refresh the choice parameters.
    if (inMapLoading)
    {
        if (vectorsAreFilled())
        {
            index_t num = index_t(m_case.varyingFields.size() + m_case.constantFields.size() + 1);
            std::vector<const char *> choiceVal;
            choiceVal = getFieldList();

            for (int i = 0; i < num_ports; ++i)
            {
                std::string lastSelection = lastDataPortSelection[i];
                int last = 0;
                for (int j = 0; j < choiceVal.size(); ++j)
                {
                    if (strcmp(lastSelection.c_str(), choiceVal[j]) == 0)
                    {
                        last = j;
                        break;
                    }
                }
                portChoice[i]->setValue(num, &choiceVal[0], last);
            }

            for (int i = 0; i < num_boundary_data_ports; ++i)
            {
                std::string lastSelection = lastBoundaryPortSelection[i];
                int last = 0;
                for (int j = 0; j < choiceVal.size(); ++j)
                {
                    if (strcmp(lastSelection.c_str(), choiceVal[j]) == 0)
                    {
                        last = j;
                        break;
                    }
                }
                boundaryDataChoice[i]->setValue(num, &choiceVal[0], last);
            }

            for (int i = 0; i < num_ports; ++i)
            {
                std::string lastSelection = lastParticlePortSelection[i];
                int last = 0;
                for (int j = 0; j < choiceVal.size(); ++j)
                {
                    if (strcmp(lastSelection.c_str(), choiceVal[j]) == 0)
                    {
                        last = j;
                        break;
                    }
                }
                particleDataChoice[i]->setValue(num, &choiceVal[0], last);
            }
        }
    }
}

coDoUnstructuredGrid *ReadFOAM::loadMesh(const std::string &meshdir,
                                         const std::string &pointsdir,
                                         const std::string &meshObjName,
                                         const index_t Processor)
{
    coDoUnstructuredGrid *meshObj;
    index_t *el, *cl, *tl; // element list, connectivity list, type list
    float *x_coord, *y_coord, *z_coord; // coordinate lists

    if (Processor == -1)
    {
        std::shared_ptr<std::istream> pointsIn = m_case.getStreamForFile(pointsdir, "points");
        if (!pointsIn)
            return NULL;
        HeaderInfo pointsH = readFoamHeader(*pointsIn);
        std::cerr << std::time(0) << " Reading mesh from:                 " << meshdir.c_str() << std::endl;
        {
            int num_points = pointsH.lines;
            //std::cerr << std::time(0) << " reading Faces" << std::endl;
            std::shared_ptr<std::istream> facesIn = m_case.getStreamForFile(meshdir, "faces");
            if (!facesIn)
                return NULL;
            HeaderInfo facesH = readFoamHeader(*facesIn);
            std::vector<std::vector<index_t> > faces(facesH.lines);
            readIndexListArray(facesH, *facesIn, faces.data(), faces.size());

            //std::cerr << std::time(0) << " reading Owners" << std::endl;
            std::shared_ptr<std::istream> ownersIn = m_case.getStreamForFile(meshdir, "owner");
            if (!ownersIn)
                return NULL;
            HeaderInfo ownerH = readFoamHeader(*ownersIn);
            DimensionInfo dim = parseDimensions(ownerH.note);
            std::vector<index_t> owners(ownerH.lines);
            readIndexArray(ownerH, *ownersIn, owners.data(), owners.size());

            //std::cerr << std::time(0) << " reading neighbours" << std::endl;
            std::shared_ptr<std::istream> neighborsIn = m_case.getStreamForFile(meshdir, "neighbour");
            if (!neighborsIn)
                return NULL;
            HeaderInfo neighbourH = readFoamHeader(*neighborsIn);
            if (neighbourH.lines != dim.internalFaces)
            {
                std::cerr << "inconsistency: #internalFaces != #neighbours" << std::endl;
                std::cerr << " #internalFaces = " << dim.internalFaces << std::endl;
                std::cerr << " #neighbours = " << neighbourH.lines << std::endl;
            }
            std::vector<index_t> neighbours(neighbourH.lines);
            readIndexArray(neighbourH, *neighborsIn, neighbours.data(), neighbours.size());

            //mesh
            //std::cerr << std::time(0) << " creating cellToFace Mapping" << std::endl;
            std::vector<std::vector<index_t> > cellfacemap(dim.cells);
            for (index_t face = 0; face < owners.size(); ++face)
            {
                cellfacemap[owners[face]].push_back(face);
            }

            for (index_t face = 0; face < neighbours.size(); ++face)
            {
                cellfacemap[neighbours[face]].push_back(face);
            }

            //std::cerr << std::time(0) << " Adding up connectivities" << std::endl;
            index_t num_elem = dim.cells;
            std::vector<index_t> types(num_elem, 0);
            index_t num_conn = 0;
            index_t num_hex = 0, num_tet = 0, num_prism = 0, num_pyr = 0, num_poly = 0;
            //Check Shape of Cells and add fill Type_List
            for (index_t i = 0; i < num_elem; i++)
            {
                const std::vector<index_t> &cellfaces = cellfacemap[i];
                const vertex_set cellvertices = getVerticesForCell(cellfaces, faces);
                bool onlySimpleFaces = true; //Simple Face = Triangle or Square
                for (index_t j = 0; j < cellfaces.size(); ++j)
                { //check if Cell has only Triangular and/or Square Faces
                    if (faces[cellfaces[j]].size() < 3 || faces[cellfaces[j]].size() > 4)
                    {
                        onlySimpleFaces = false;
                        break;
                    }
                }
                const index_t num_faces = index_t(cellfaces.size());
                index_t num_verts = index_t(cellvertices.size());
                if (num_faces == 6 && num_verts == 8 && onlySimpleFaces)
                {
                    types[i] = TYPE_HEXAEDER;
                    ++num_hex;
                }
                else if (num_faces == 5 && num_verts == 6 && onlySimpleFaces)
                {
                    types[i] = TYPE_PRISM;
                    ++num_prism;
                }
                else if (num_faces == 5 && num_verts == 5 && onlySimpleFaces)
                {
                    types[i] = TYPE_PYRAMID;
                    ++num_pyr;
                }
                else if (num_faces == 4 && num_verts == 4 && onlySimpleFaces)
                {
                    types[i] = TYPE_TETRAHEDER;
                    ++num_tet;
                }
                else
                {
                    ++num_poly;
                    types[i] = TYPE_POLYHEDRON;
                    num_verts = 0;
                    for (index_t j = 0; j < cellfaces.size(); ++j)
                    {
                        num_verts += index_t(faces[cellfaces[j]].size() + 1);
                    }
                }
                num_conn += num_verts;
            }

            //Create the unstructured grid
            meshObj = new coDoUnstructuredGrid(meshObjName.c_str(), num_elem, num_conn, num_points, 1);

            // get pointers to the first element of the element, vertex and coordinate lists
            meshObj->getAddresses(&el, &cl, &x_coord, &y_coord, &z_coord);
            // get a pointer to the type list
            meshObj->getTypeList(&tl);

            //std::cerr << std::time(0) << " Setting element list and connectivity list" << std::endl;
            // save data cell by cell to element, connectivity and type list
            index_t conncount = 0;
            std::vector<index_t> connectivities;
            //go cell by cell (element by element)
            for (index_t i = 0; i < dim.cells; i++)
            {
                //element list
                *el = conncount;
                ++el;
                //connectivity list
                const std::vector<index_t> &cellfaces = cellfacemap[i]; //get all faces of current cell
                //IF cell is Hexahedron
                if (types[i] == TYPE_HEXAEDER)
                {
                    index_t ia = cellfaces[0]; //Pick the first face in the Vector as Starting Face (all faces are squares)
                    std::vector<index_t> a = faces[ia]; //find face that corresponds to index ia

                    bool na = isPointingInwards(ia, i, dim.internalFaces, owners, neighbours);
                    if (na == false)
                    { //if normal vector is not pointing inwards
                        std::reverse(a.begin(), a.end()); //reverse the ordering of the Vertices
                    }

                    connectivities = a;
                    connectivities.push_back(findVertexAlongEdge(a[0], ia, cellfaces, faces));
                    connectivities.push_back(findVertexAlongEdge(a[1], ia, cellfaces, faces));
                    connectivities.push_back(findVertexAlongEdge(a[2], ia, cellfaces, faces));
                    connectivities.push_back(findVertexAlongEdge(a[3], ia, cellfaces, faces));

                    conncount += 8;
                }

                if (types[i] == TYPE_PRISM)
                {
                    index_t it = 1;
                    index_t ia = cellfaces[0];
                    while (faces[ia].size() > 3)
                    { //find triangular face and use it as starting face
                        ia = cellfaces[it++];
                    }

                    std::vector<index_t> a = faces[ia];

                    bool na = isPointingInwards(ia, i, dim.internalFaces, owners, neighbours);
                    if (na == false)
                    {
                        std::reverse(a.begin(), a.end());
                    }

                    connectivities = a;
                    connectivities.push_back(findVertexAlongEdge(a[0], ia, cellfaces, faces));
                    connectivities.push_back(findVertexAlongEdge(a[1], ia, cellfaces, faces));
                    connectivities.push_back(findVertexAlongEdge(a[2], ia, cellfaces, faces));

                    conncount += 6;
                }

                if (types[i] == TYPE_PYRAMID)
                {
                    index_t it = 1;
                    index_t ia = cellfaces[0];
                    while (faces[ia].size() < 4)
                    { //find the square and use it as starting face
                        ia = cellfaces[it++];
                    }

                    std::vector<index_t> a = faces[ia];

                    bool na = isPointingInwards(ia, i, dim.internalFaces, owners, neighbours);
                    if (na == false)
                    {
                        std::reverse(a.begin(), a.end());
                    }

                    connectivities = a;
                    connectivities.push_back(findVertexAlongEdge(a[0], ia, cellfaces, faces));

                    conncount += 5;
                }

                if (types[i] == TYPE_TETRAHEDER)
                {
                    index_t ia = cellfaces[0]; //use first face in vector as starting face (all faces are triangles)
                    std::vector<index_t> a = faces[ia];

                    bool na = isPointingInwards(ia, i, dim.internalFaces, owners, neighbours);
                    if (na == false)
                    {
                        std::reverse(a.begin(), a.end());
                    }

                    connectivities = a;
                    connectivities.push_back(findVertexAlongEdge(a[0], ia, cellfaces, faces));

                    conncount += 4;
                }

                if (types[i] == TYPE_POLYHEDRON)
                {
                    index_t kk;
                    for (index_t j = 0; j < cellfaces.size(); j++)
                    { //go through all faces in order
                        index_t ia = cellfaces[j];
                        std::vector<index_t> a = faces[ia];

                        bool na = isPointingInwards(ia, i, dim.internalFaces, owners, neighbours);

                        if (na == false)
                        {
                            std::reverse(a.begin(), a.end());
                        }
                        for (index_t k = 0; k < a.size() + 1; k++)
                        { //go through the vertices of the current face in order
                            if (k == a.size())
                            {
                                kk = 0;
                            }
                            else
                            {
                                kk = k;
                            } //the first point has to appear again at the end
                            connectivities.push_back(a[kk]);
                            conncount++;
                        }
                    }
                }

                for (index_t j = 0; j < connectivities.size(); j++)
                { //add connectivities of the current element to the connectivity List
                    *cl = connectivities[j];
                    ++cl;
                }
                // add the type of the current element the type lists
                *tl++ = types[i];

                connectivities.clear();
            }
        }

        // save coordinates to coordinate lists
        //      if (pointsH.lines != dim.points) {
        //         std::cerr << std::time(0) << " inconsistency: #Number of points in points-file != #points declared in owner header" << std::endl;
        //      }
        std::cerr << std::time(0) << " Reading points from:               " << pointsdir.c_str() << std::endl;
        //std::cerr << std::time(0) << " reading Points" << std::endl;
        readFloatVectorArray(pointsH, *pointsIn, x_coord, y_coord, z_coord, pointsH.lines);
    }
    else
    { //if Processor >= 0  ->  Copy everything but the Coordinates from basemeshs[processor]
        std::cerr << std::time(0) << " copying mesh from most recent timestep in Processor" << Processor << std::endl;
        coDoUnstructuredGrid *oldMesh = basemeshs[Processor];
        index_t num_elem, num_conn, void_points;
        oldMesh->getGridSize(&num_elem, &num_conn, &void_points);
        index_t *oldel, *oldcl, *oldtl;
        float *oldx_coord, *oldy_coord, *oldz_coord;
        oldMesh->getAddresses(&oldel, &oldcl, &oldx_coord, &oldy_coord, &oldz_coord);
        oldMesh->getTypeList(&oldtl);

        //std::cerr << std::time(0) << " Reading points from: " << pointsdir.c_str() << std::endl;
        std::shared_ptr<std::istream> pointsIn = m_case.getStreamForFile(pointsdir, "points");
        if (!pointsIn)
            return NULL;
        HeaderInfo pointsH = readFoamHeader(*pointsIn);
        int num_points = pointsH.lines;

        meshObj = new coDoUnstructuredGrid(meshObjName.c_str(), num_elem, num_conn, num_points, 1);
        // get pointers to the first element of the element, vertex and coordinate lists
        meshObj->getAddresses(&el, &cl, &x_coord, &y_coord, &z_coord);
        // get a pointer to the type list
        meshObj->getTypeList(&tl);

        for (int i = 0; i < num_elem; ++i)
        {
            el[i] = oldel[i];
            tl[i] = oldtl[i];
        }
        for (int i = 0; i < num_conn; ++i)
        {
            cl[i] = oldcl[i];
        }

        // save coordinates to coordinate lists
        std::cerr << std::time(0) << " Reading points from:               " << pointsdir.c_str() << std::endl;
        //if (pointsH.lines != dim.points) {
        //   std::cerr << std::time(0) << " inconsistency: #Number of points in points-file != #points declared in owner header" << std::endl;
        //}
        readFloatVectorArray(pointsH, *pointsIn, x_coord, y_coord, z_coord, pointsH.lines);
    }

    //std::cerr << std::time(0) << " done!" << std::endl;

    return meshObj;
}

coDoPolygons *ReadFOAM::loadPatches(const std::string &meshdir,
                                    const std::string &pointsdir,
                                    const std::string &boundObjName,
                                    const std::string &selection,
                                    const index_t Processor,
                                    const index_t saveMapTo)
{
    coDoPolygons *polyObj = NULL;
    if (Processor == -1)
    { //when Processor = -1, the boundary will be read completely
        std::cerr << std::time(0) << " Reading boundary from:             " << meshdir.c_str() << std::endl;
        std::shared_ptr<std::istream> facesIn = m_case.getStreamForFile(meshdir, "faces");
        if (!facesIn)
            return NULL;
        HeaderInfo facesH = readFoamHeader(*facesIn);
        std::vector<std::vector<index_t> > faces(facesH.lines);
        readIndexListArray(facesH, *facesIn, faces.data(), faces.size());

        coRestraint res;
        res.add(selection.c_str());
        Boundaries boundaries = m_case.loadBoundary(meshdir);
        int num_corners = 0;
        int num_polygons = 0;
        int num_points = 0;
        //creating the pointmap
        //The Key-Part first gets populated with the indexes of all the vertices that are required for the polygon
        map<int, int> pointmap;
        for (std::vector<Boundary>::const_iterator it = boundaries.boundaries.begin();
             it != boundaries.boundaries.end();
             ++it)
        {
            int boundaryIndex = it->index;
            if (res(boundaryIndex) || strcmp(selection.c_str(), "all") == 0)
            {
                for (index_t i = it->startFace; i < it->startFace + it->numFaces; ++i)
                {
                    ++num_polygons;
                    std::vector<index_t> &face = faces[i];
                    for (index_t j = 0; j < face.size(); ++j)
                    {
                        pointmap[face[j]] += 1; //it does not matter what value is assigned just that the key is created if it was not already
                        num_corners++;
                    }
                }
            }
        }
        num_points = int(pointmap.size());
        //then the valuepart is assigned incremental values starting at 0
        index_t ni = 0;
        for (std::map<int, int>::iterator it = pointmap.begin(); it != pointmap.end(); ++it)
        {
            it->second = ni;
            ++ni;
        }
        polyObj = new coDoPolygons(boundObjName.c_str(), num_points, num_corners, num_polygons);

        index_t *cornerList, *polygonList;
        float *x_start, *y_start, *z_start;
        polyObj->getAddresses(&x_start, &y_start, &z_start, &cornerList, &polygonList);

        index_t cornercount = 0;
        for (std::vector<Boundary>::const_iterator it = boundaries.boundaries.begin();
             it != boundaries.boundaries.end();
             ++it)
        {
            int boundaryIndex = it->index;
            if (res(boundaryIndex) || strcmp(selection.c_str(), "all") == 0)
            {
                for (index_t i = it->startFace; i < it->startFace + it->numFaces; ++i)
                {
                    *polygonList = cornercount;
                    ++polygonList;
                    std::vector<index_t> &face = faces[i];
                    for (index_t j = 0; j < face.size(); j++)
                    {
                        *cornerList = pointmap[face[j]];
                        ++cornerList;
                        ++cornercount;
                    }
                }
            }
        }
        std::vector<std::vector<index_t> >().swap(faces); //Deallocate the memory

        std::shared_ptr<std::istream> pointsIn = m_case.getStreamForFile(pointsdir, "points");
        if (!pointsIn)
            return NULL;
        HeaderInfo pointsH = readFoamHeader(*pointsIn);
        std::vector<scalar_t> x_coord(pointsH.lines), y_coord(pointsH.lines), z_coord(pointsH.lines);
        readFloatVectorArray(pointsH, *pointsIn, x_coord.data(), y_coord.data(), z_coord.data(), pointsH.lines);
        for (std::map<int, int>::iterator it = pointmap.begin(); it != pointmap.end(); ++it)
        {
            *x_start = x_coord[it->first];
            ++x_start;
            *y_start = y_coord[it->first];
            ++y_start;
            *z_start = z_coord[it->first];
            ++z_start;
        }

        std::vector<scalar_t>().swap(x_coord); //Deallocate the Memory
        std::vector<scalar_t>().swap(y_coord);
        std::vector<scalar_t>().swap(z_coord);

        if (saveMapTo > -1)
        { //if the saveMapTo Parameter is set it will save the pointsmap to a global vector for later use
            pointmaps[saveMapTo] = pointmap;
        }
    }
    else
    { //when processor is not -1 the polygons will be copied from the first timestep of the specified processor
        std::map<int, int> pointmap = pointmaps[Processor];
        coDoPolygons *oldBoundary = basebounds[Processor];
        index_t *oldCornerList, *oldPolygonList;
        float *oldX_start, *oldY_start, *oldZ_start;
        oldBoundary->getAddresses(&oldX_start, &oldY_start, &oldZ_start, &oldCornerList, &oldPolygonList);
        index_t num_corners = oldBoundary->getNumVertices(), num_polygons = oldBoundary->getNumPolygons(), num_points = oldBoundary->getNumPoints();

        polyObj = new coDoPolygons(boundObjName.c_str(), num_points, num_corners, num_polygons);
        index_t *cornerList, *polygonList;
        float *x_start, *y_start, *z_start;
        polyObj->getAddresses(&x_start, &y_start, &z_start, &cornerList, &polygonList);

        for (int i = 0; i < num_polygons; ++i)
        {
            polygonList[i] = oldPolygonList[i];
        }
        for (int i = 0; i < num_corners; ++i)
        {
            cornerList[i] = oldCornerList[i];
        }

        // save coordinates to coordinate lists
        std::cerr << std::time(0) << " copying Boundary Polygons and reading new points from: " << pointsdir.c_str() << std::endl;
        std::shared_ptr<std::istream> pointsIn = m_case.getStreamForFile(pointsdir, "points");
        if (!pointsIn)
            return NULL;
        HeaderInfo pointsH = readFoamHeader(*pointsIn);
        std::vector<scalar_t> x_coord(pointsH.lines), y_coord(pointsH.lines), z_coord(pointsH.lines);
        readFloatVectorArray(pointsH, *pointsIn, x_coord.data(), y_coord.data(), z_coord.data(), pointsH.lines);

        for (std::map<int, int>::iterator it = pointmap.begin(); it != pointmap.end(); ++it)
        {
            *x_start = x_coord[it->first];
            ++x_start;
            *y_start = y_coord[it->first];
            ++y_start;
            *z_start = z_coord[it->first];
            ++z_start;
        }
    }
    //std::cerr << std::time(0) << " done!" << std::endl;

    return polyObj;
}

coDoPoints *ReadFOAM::loadParticles(const std::string &datadir, const std::string &objName, const std::string &cellIdObjName, coDistributedObject **cellIdsPointer)
{
    *cellIdsPointer = NULL;
    std::string lagdir = datadir + "/lagrangian/" + m_case.lagrangiandir;

    std::shared_ptr<std::istream> posIn = m_case.getStreamForFile(lagdir, "positions");
    if (!posIn)
        return NULL;
    HeaderInfo posH = readFoamHeader(*posIn);
    std::cerr << std::time(0) << " Reading positions from " << lagdir << std::endl;
    std::cerr << "lines: " << posH.lines << ", type: " << posH.fieldclass << std::endl;
    int num_pos = posH.lines;
    float *x=NULL, *y=NULL, *z=NULL;
    coDoPoints *pointObj = NULL;
    if (!objName.empty())
    {
        pointObj = new coDoPoints(objName.c_str(), num_pos);
        pointObj->getAddresses(&x, &y, &z);
    }
    coDoInt *cellIds = NULL;
    int *cell = NULL;
    if (!cellIdObjName.empty())
    {
        cellIds = new coDoInt(cellIdObjName, num_pos);
        cell = cellIds->getAddress();
    }
    if (!readParticleArray(posH, *posIn, x, y, z, cell, num_pos))
    {
        delete pointObj;
        delete cellIds;
        return NULL;
    }

    *cellIdsPointer = cellIds;
    return pointObj;
}


coDistributedObject *ReadFOAM::loadField(const std::string &timedir,
                                     const std::string &file,
                                     const std::string &vecObjName,
                                     const std::string &meshdir)
{
    std::shared_ptr<std::istream> vecIn = m_case.getStreamForFile(timedir, file);
    if (!vecIn)
        return NULL;
    HeaderInfo header = readFoamHeader(*vecIn);
    size_t numberCells = header.lines;
    if (numberCells == 0)
    {
        std::shared_ptr<std::istream> ownersIn = m_case.getStreamForFile(meshdir, "owner");
        HeaderInfo ownerH = readFoamHeader(*ownersIn);
        DimensionInfo dim = parseDimensions(ownerH.note);
        numberCells = dim.cells;
    }
    coDistributedObject *fieldObj;
    std::cerr << std::time(0) << "Field with name " << header.object.c_str() << " has dimensions " << header.dimensions.c_str() << std::endl;
    if (header.fieldclass == "volVectorField" || header.fieldclass == "vectorField")
    {
        std::cerr << std::time(0) << " Reading VectorField from:          " << timedir.c_str() << "/" << file.c_str() << std::endl;
        coDoVec3 *vecObj = new coDoVec3(vecObjName.c_str(), int(numberCells));
        float *x, *y, *z;
        vecObj->getAddresses(&x, &y, &z);
        if (header.lines==0)
        {
            for (int i=0; i<numberCells; ++i)
            {
                x[i] = 0.0;
                y[i] = 0.0;
                z[i] = 0.0;
            }
        }
        else
        {
            readFloatVectorArray(header, *vecIn, x, y, z, header.lines);
        }
        fieldObj= vecObj;
    }
    else if (header.fieldclass == "volScalarField" || header.fieldclass == "scalarField")
    {
        std::cerr << std::time(0) << " Reading ScalarField from:          " << timedir.c_str() << "/" << file.c_str() << std::endl;
        coDoFloat *vecObj = new coDoFloat(vecObjName.c_str(), int(numberCells));
            float *x=vecObj->getAddress();
        if (header.lines==0)
        {
            float uniformFieldValue=float(std::atof(header.internalField.c_str()));
            for (int i=0; i<numberCells; ++i)
            {
                x[i] = uniformFieldValue;
            }
        }
        else
        { 
            vecObj->getAddress(&x);
            readFloatArray(header, *vecIn, x, header.lines);
        }
        fieldObj= vecObj;
    }
    else if (header.fieldclass == "labelField")
    {
        std::cerr << std::time(0) << " Reading labelField from:" << timedir.c_str() << "//" << file.c_str() << std::endl;
        coDoInt *vecObj = new coDoInt(vecObjName.c_str(), int(numberCells));
        int *d=vecObj->getAddress();
        if (header.lines==0)
        {
            int uniformFieldValue=std::atoi(header.internalField.c_str());
            for (int i=0; i<numberCells; ++i)
            {
                d[i] = uniformFieldValue;
            }
        }
        else
        {
            readIndexArray(header, *vecIn, d, header.lines);
        }
        fieldObj= vecObj;
    }
    else
    {
        std::cerr << "Unknown field type in file: " << file.c_str() << std::endl;
        return NULL;
    }
    return fieldObj;
    //std::cerr << std::time(0) << " done!" << std::endl;
}

coDistributedObject *ReadFOAM::loadBoundaryField(const std::string &timedir,
                                            const std::string &meshdir,
                                            const std::string &file,
                                            const std::string &vecObjName,
                                            const std::string &selection)
{

    std::shared_ptr<std::istream> ownersIn = m_case.getStreamForFile(meshdir, "owner");
    if (!ownersIn)
        return NULL;
    HeaderInfo ownerH = readFoamHeader(*ownersIn);
    std::vector<index_t> owners(ownerH.lines);
    readIndexArray(ownerH, *ownersIn, owners.data(), owners.size());

    coRestraint res;
    res.add(selection.c_str());
    Boundaries boundaries = m_case.loadBoundary(meshdir);
    std::vector<index_t> dataMapping;
    int numBoundaryFaces =0;
    std::shared_ptr<std::istream> vecIn = m_case.getStreamForFile(timedir, file);
    if (!vecIn)
        return NULL;
    HeaderInfo header = readFoamHeader(*vecIn);
    for (std::vector<Boundary>::const_iterator it = boundaries.boundaries.begin();
            it != boundaries.boundaries.end();
            ++it)
    {
        int boundaryIndex = it->index;
        if (res(boundaryIndex) || strcmp(selection.c_str(), "all") == 0)
        {
            if (header.lines!=0)
            {
                for (index_t i = it->startFace; i < it->startFace + it->numFaces; ++i)
                {
                    dataMapping.push_back(owners[i]);
                }
            }
            numBoundaryFaces+= it->numFaces;
        }
    }

    coDistributedObject *fieldObj = NULL;

    if (header.fieldclass == "volScalarField" || header.fieldclass == "scalarField")
    {
        std::cerr << std::time(0) << " Reading Boundary ScalarField from: " << timedir.c_str() << "//" << file.c_str() << std::endl;

        coDoFloat *vecObj = new coDoFloat(vecObjName.c_str(), numBoundaryFaces);
        float *x = vecObj->getAddress();

        if (header.lines==0)
        {
            float uniformFieldValue=float(std::atof(header.internalField.c_str()));
            for (int i=0; i<numBoundaryFaces; ++i)
            {
                x[i] = uniformFieldValue;
            }
        }
        else
        {
            std::vector<scalar_t> fullX(header.lines);
            readFloatArray(header, *vecIn, fullX.data(), header.lines);
            for (index_t i = 0; i < dataMapping.size(); ++i)
            {
                *x = fullX[dataMapping[i]];
                ++x;
            }
        }
        fieldObj=vecObj;
    }
    else if (header.fieldclass == "volVectorField" || header.fieldclass == "vectorField")
    {
        std::cerr << std::time(0) << " Reading Boundary VectorField from: " << timedir.c_str() << "//" << file.c_str() << std::endl;


        coDoVec3 *vecObj = new coDoVec3(vecObjName.c_str(), numBoundaryFaces);
        float *x, *y, *z;
        vecObj->getAddresses(&x, &y, &z);

        if (header.lines==0)
        {
            for (int i=0; i<numBoundaryFaces; ++i)
            {
                x[i] = 0.0;
                y[i] = 0.0;
                z[i] = 0.0;
            }
        }
        else
        {
            std::vector<scalar_t> fullX(header.lines), fullY(header.lines), fullZ(header.lines);
            readFloatVectorArray(header, *vecIn, fullX.data(), fullY.data(), fullZ.data(), header.lines);
            for (index_t i = 0; i < dataMapping.size(); ++i)
            {
                *x = fullX[dataMapping[i]];
                ++x;
                *y = fullY[dataMapping[i]];
                ++y;
                *z = fullZ[dataMapping[i]];
                ++z;
            }
        }
        fieldObj = vecObj;
    }
    else
    {
        std::cerr << "Unknown field type in file: " << file.c_str() << std::endl;
        return NULL;
    }
    //std::cerr << std::time(0) << " done!" << std::endl;
    return fieldObj;
}


int ReadFOAM::compute(const char *port) //Compute is called when Module is executed
{
    (void)port;
    m_case = getCaseInfo(casedir);
    if (m_case.timedirs.size()==0) // create dummy timestep to read mesh at least once and create processorID data port if selected 
    {
        std::string bn = "0";
        double t = atof(bn.c_str()); 
        num_boundary_data_ports=0;
        if (portChoice[0]->getValue()==1)
        {
            num_ports=1;
        }
        else
            num_ports=0;
        m_case.timedirs[t] = bn;
	m_case.completeMeshDirs[t] = m_case.constantdir;
    }
    //Mesh
    int skipfactor = skipfactorParam->getValue();
    if (skipfactor < 1)
    {
        skipfactor = 1;
        skipfactorParam->setValue(1);
    }
    float starttime = starttimeParam->getValue();
    float stoptime = stoptimeParam->getValue(); 
    basemeshs.clear();
    basebounds.clear();
    pointmaps.clear();
    pointmaps.resize(std::max(1,m_case.numblocks));

    std::vector<coDistributedObject *> meshSubSets;
    coDoSet *meshSet=NULL, *meshSubSet=NULL;
    std::vector<coDistributedObject *> boundarySubSets;
    coDoSet *boundarySet=NULL, *boundarySubSet=NULL;
    std::vector<coDistributedObject *> particleSubSets;
    coDoSet *particleSet=NULL, *particleSubSet=NULL;
    std::vector<std::vector<coDistributedObject *> > portSubSets(num_ports);
    coDoSet *portSubSet=NULL;
    std::vector<std::vector<coDistributedObject *> > boundPortSubSets(num_boundary_data_ports);
    coDoSet *boundPortSubSet=NULL;
    std::vector<std::vector<coDistributedObject *> > particlePortSubSets(num_ports);
    coDoSet *particlePortSubSet=NULL;

    std::vector <std::string> lastmeshdir(std::max(1,m_case.numblocks));
    std::vector <std::string> lastbounddir(std::max(1,m_case.numblocks));

    std::vector<size_t> cellIdPorts;
    for (int nPort = 0; nPort < num_ports; ++nPort)
    {
        index_t portchoice = particleDataChoice[nPort]->getValue();
        if (portchoice == 1)
        {
            cellIdPorts.push_back(nPort);
        }
    }

    int counter = 0;
    index_t timestep = 0;
    for (std::map<double, std::string>::const_iterator it = m_case.timedirs.begin();
            it != m_case.timedirs.end();
            ++it)
    {
        double t = it->first;
        const std::string &timedir = it->second;
        if (t >= starttime && t <= (stoptime*(1+1e-6)))
        { 
            std::stringstream realtime_s;
            realtime_s << t;
            std::string realtime = realtime_s.str();
            if (counter % skipfactor == 0)
            {
                if (meshParam->getValue() || boundaryParam->getValue() || true
                        || (m_case.hasParticles && (particleParam->getValue() || !cellIdPorts.empty())))
                {
                    coModule::sendInfo("Reading mesh/boundary and data. Please wait ...");
                    std::cerr << std::time(0) << " Reading mesh, boundary and fields for timestep: " << t  << " (dir=" << timedir << ")" << std::endl;
                    std::string selection = patchesStringParam->getValString();

                    std::vector<coDistributedObject *> tempSetMesh;
                    std::vector<coDistributedObject *> tempSetBoundary;
                    std::vector<coDistributedObject *> tempSetParticles;
                    std::vector<std::vector<coDistributedObject *> > tempSetPort(num_ports);
                    std::vector<std::vector<coDistributedObject *> > tempSetBoundPort(num_boundary_data_ports);
                    std::vector<std::vector<coDistributedObject *> > tempSetParticlesPort(num_ports);
                    for (index_t j = 0;( j < m_case.numblocks || j==0); j++)
                    { //fill vector:tempSet with all the mesh parts of all processors even if its just one
                        //std::cerr << " processor" << j;
                        std::stringstream sMeshDir;
                        std::stringstream sPointsDir;
                        std::stringstream sDataDir;
                        if (m_case.numblocks > 0)
                        {
                            sMeshDir << "processor" << j  << "/" << m_case.completeMeshDirs[t] << "/polyMesh";
                            sPointsDir << "processor" << j  << "/" << timedir << "/polyMesh";
                            sDataDir << "processor" << j  << "/" << timedir;
                        }
                        else
                        {
                            sMeshDir << m_case.completeMeshDirs[t] << "/polyMesh";
                            sPointsDir << timedir << "/polyMesh";
                            sDataDir << timedir;
                        }

                        std::string meshdir = sMeshDir.str();
                        std::string pointsdir;
                        if (m_case.varyingCoords)
                            pointsdir = sPointsDir.str();
                        else
                            pointsdir = meshdir;
                        std::string datadir = sDataDir.str();
                        std::stringstream sm;
                        std::stringstream sb;
                        std::stringstream sd;
                        std::stringstream sp;
                        if (m_case.numblocks > 0)
                        {
                            sm << "_timestep_" << timestep << "_processor_" << j;
                            sb << "_timestep_" << timestep << "_processor_" << j;
                            sd << "_timestep_" << timestep << "_processor_" << j;
                            sp << "_timestep_" << timestep << "_processor_" << j;
                        }
                        else
                        {
                            sm << "_timestep_" << timestep << "_mesh";
                            sb << "_timestep_" << timestep << "_polygon";
                            sd << "_timestep_" << timestep << "_data";
                            sp << "_timestep_" << timestep << "_data";
                        }

                        if ((!m_case.varyingCoords && counter==0) || m_case.varyingCoords)
                        {
                            if (meshParam->getValue())
                            {   
                                std::string meshObjName = meshOutPort->getObjName();
                                meshObjName += sm.str();
                                coDoUnstructuredGrid *m = NULL;
                                if (lastmeshdir[j]==meshdir)
                                {
                                    m = loadMesh(meshdir, pointsdir, meshObjName, j);
                                }
                                else
                                {
                                    m = loadMesh(meshdir, pointsdir, meshObjName);
                                    basemeshs[j] = m;
                                    lastmeshdir[j]=meshdir;
                                }
                                if (m_case.varyingCoords)
                                    m->addAttribute("REALTIME", realtime.c_str());
                                tempSetMesh.push_back(m);
                            }
                            if (boundaryParam->getValue())
                            {
                                std::string boundObjName = boundaryOutPort->getObjName();
                                boundObjName += sb.str();
                                coDoPolygons *p = NULL;

                                if (lastbounddir[j] == meshdir)
                                {
                                    p = loadPatches(meshdir, pointsdir, boundObjName, selection,j);
                                }
                                else
                                {
                                    p = loadPatches(meshdir, pointsdir, boundObjName, selection,-1,j);
                                    basebounds[j] = p;
                                    lastbounddir[j]=meshdir;
                                }
                                if (m_case.varyingCoords)
                                    p->addAttribute("REALTIME", realtime.c_str());
                                tempSetBoundary.push_back(p);
                            }
                        }
                        for (int nPort = 0; nPort < num_ports; ++nPort)
                        {

                            index_t portchoice = portChoice[nPort]->getValue();
                            if (portchoice > 0 && portchoice <= m_case.varyingFields.size()+1)
                            {
                                std::string dataFilename = portChoice[nPort]->getLabel(portchoice);
                                if (portchoice == 1)
                                {
                                    std::shared_ptr<std::istream> ownersIn = m_case.getStreamForFile(meshdir, "owner");
                                    HeaderInfo ownerH = readFoamHeader(*ownersIn);
                                    DimensionInfo dim = parseDimensions(ownerH.note);
                                    std::string portObjName = outPorts[nPort]->getObjName();
                                    portObjName += sd.str();

                                    coDoFloat *v = new coDoFloat(portObjName, dim.cells);
                                    float *processorID = v->getAddress();

                                    for (int i=0; i<dim.cells; ++i)
                                    {
                                        processorID[i] = float(j);
                                    }
                                    v->addAttribute("REALTIME", realtime.c_str());
                                    tempSetPort[nPort].push_back(v);
                                }
                                else
                                {
                                    std::string portObjName = outPorts[nPort]->getObjName();
                                    portObjName += sd.str();

                                    coDistributedObject *v = loadField(datadir, dataFilename, portObjName, meshdir);
                                    if (v)
                                        v->addAttribute("REALTIME", realtime.c_str());
                                    tempSetPort[nPort].push_back(v);
                                }
                            }                           
                        }
                        for (int nPort = 0; nPort < num_boundary_data_ports; ++nPort)
                        {
                            index_t portchoice = boundaryDataChoice[nPort]->getValue();
                            if (portchoice > 0 && portchoice <= m_case.varyingFields.size()+1)
                            {
                                std::string dataFilename = portChoice[nPort]->getLabel(portchoice);
                                if (portchoice == 1)
                                {//ToDo:Replace dim.cells with the correct number of faces
                                    coRestraint res;
                                    res.add(selection.c_str());
                                    Boundaries boundaries = m_case.loadBoundary(meshdir);
                                    int numBoundaryFaces =0;
                                    for (std::vector<Boundary>::const_iterator it = boundaries.boundaries.begin();
                                            it != boundaries.boundaries.end();
                                            ++it)
                                    {
                                        int boundaryIndex = it->index;
                                        if (res(boundaryIndex) || strcmp(selection.c_str(), "all") == 0)
                                        {
                                            numBoundaryFaces+= it->numFaces;
                                        }
                                    }
                                    std::string portObjName = boundaryDataPorts[nPort]->getObjName();
                                    portObjName += sd.str();

                                    coDoFloat *v = new coDoFloat(portObjName, numBoundaryFaces);
                                    float *processorID = v->getAddress();

                                    for (int i=0; i<numBoundaryFaces; ++i)
                                    {
                                        processorID[i] = float(j);
                                    }
                                    v->addAttribute("REALTIME", realtime.c_str());
                                    tempSetBoundPort[nPort].push_back(v);
                                }
                                else
                                {
                                    std::string portObjName = boundaryDataPorts[nPort]->getObjName();
                                    portObjName += sd.str();

                                    coDistributedObject *v = loadBoundaryField(datadir, meshdir, dataFilename, portObjName, selection);
                                    if (v)
                                        v->addAttribute("REALTIME", realtime.c_str());
                                    tempSetBoundPort[nPort].push_back(v);
                                }
                            }
                        }

                        // lagrangian/particle data
                        if (m_case.hasParticles)
                        {
                            if (particleParam->getValue() || !cellIdPorts.empty())
                            {
                                std::string particleObjName, cellIdObjName;
                                if (particleParam->getValue())
                                {
                                    particleObjName = particleOutPort->getObjName();
                                    particleObjName += sp.str();
                                }
                                if (!cellIdPorts.empty())
                                {
                                    cellIdObjName = particleDataPorts[cellIdPorts[0]]->getObjName();
                                    cellIdObjName += sp.str();
                                }
                                coDistributedObject *cellIds = NULL;
                                coDoPoints *p = loadParticles(datadir, particleObjName, cellIdObjName, &cellIds);
                                if (p)
                                {
                                    p->addAttribute("REALTIME", realtime.c_str());
                                    tempSetParticles.push_back(p);
                                }
                                if (cellIds)
                                {
                                    cellIds->addAttribute("REALTIME", realtime.c_str());
                                    for (size_t i=0; i<cellIdPorts.size(); ++i)
                                        tempSetParticlesPort[cellIdPorts[i]].push_back(cellIds);
                                }
                            }
                            for (int nPort = 0; nPort < num_ports; ++nPort)
                            {
                                index_t portchoice = particleDataChoice[nPort]->getValue();
                                if (portchoice > 1 && portchoice <= m_case.particleFields.size()+1)
                                {
                                    std::string lagdir = datadir + "/lagrangian/" + m_case.lagrangiandir;
                                    std::string dataFilename = particleDataChoice[nPort]->getLabel(portchoice);
                                    std::string portObjName = particleDataPorts[nPort]->getObjName();
                                    portObjName += sp.str();

                                    coDistributedObject *v = loadField(lagdir, dataFilename, portObjName, meshdir);
                                    if (v)
                                        v->addAttribute("REALTIME", realtime.c_str());
                                    tempSetParticlesPort[nPort].push_back(v);
                                }
                            }
                        }
                    }
                    std::stringstream s;
                    s << "_set_timestep" << timestep;
                    if ((!m_case.varyingCoords && counter==0) || m_case.varyingCoords)
                    {
                        if (meshParam->getValue())
                        {
                            //std::cerr << std::time(0) << " Read mesh for timestep: " << t << std::endl;
                            std::string meshSubSetName = meshOutPort->getObjName();
                            meshSubSetName += s.str();
                            meshSubSet = new coDoSet(meshSubSetName, int(tempSetMesh.size()), &tempSetMesh.front());
                            if (m_case.varyingCoords)
                                meshSubSet->addAttribute("REALTIME", realtime.c_str());
                            meshSubSets.push_back(meshSubSet);
                        }
                        if (boundaryParam->getValue())
                        {
                            //std::cerr << std::time(0) << " Read boundary for timestep: " << t << std::endl;
                            std::string boundSubSetName = boundaryOutPort->getObjName();
                            boundSubSetName += s.str();
                            boundarySubSet = new coDoSet(boundSubSetName, int(tempSetBoundary.size()), &tempSetBoundary.front());
                            if (m_case.varyingCoords)
                                boundarySubSet->addAttribute("REALTIME", realtime.c_str());
                            boundarySubSets.push_back(boundarySubSet);
                        }
                    }
                    else if (!m_case.varyingCoords && counter>0)
                    { //more than 1 timestep and none-changing mesh-> mesh needs to be referenced multiple times
                        if (meshParam->getValue())
                        {
                            meshSubSets.push_back(meshSubSet);
                            meshSubSet->incRefCount();
                        }
                        if (boundaryParam->getValue())
                        {
                            boundarySubSets.push_back(boundarySubSet);
                            boundarySubSet->incRefCount();
                        }
                    }

                    for (int nPort = 0; nPort < num_ports; ++nPort)
                    {
                        index_t portchoice = portChoice[nPort]->getValue();
                        if (portchoice > 0 && portchoice <= m_case.varyingFields.size()+1)
                        {
                            //std::cerr << std::time(0) << " Read data for timestep: " << t << std::endl;
                            std::string portSubSetName = outPorts[nPort]->getObjName();
                            portSubSetName += s.str();
                            portSubSet = new coDoSet(portSubSetName, int(tempSetPort[nPort].size()), &tempSetPort[nPort].front());
                            portSubSet->addAttribute("REALTIME", realtime.c_str());
                            portSubSets[nPort].push_back(portSubSet);
                        }
                    }
                    for (int nPort = 0; nPort < num_boundary_data_ports; ++nPort)
                    {
                        index_t portchoice = boundaryDataChoice[nPort]->getValue();
                        if (portchoice > 0 && portchoice <= m_case.varyingFields.size()+1)
                        {
                            //std::cerr << std::time(0) << " Read boundary data for timestep: " << t << std::endl;
                            std::string boundPortSubSetName = boundaryDataPorts[nPort]->getObjName();
                            boundPortSubSetName += s.str();
                            boundPortSubSet = new coDoSet(boundPortSubSetName, int(tempSetBoundPort[nPort].size()), &tempSetBoundPort[nPort].front());
                            boundPortSubSet->addAttribute("REALTIME", realtime.c_str());
                            boundPortSubSets[nPort].push_back(boundPortSubSet);
                        }
                    }

                    // lagrangian/particle data
                    if (m_case.hasParticles && particleParam->getValue())
                    {
                        std::string particleSubSetName = particleOutPort->getObjName();
                        particleSubSetName += s.str();
                        particleSubSet = new coDoSet(particleSubSetName, int(tempSetParticles.size()), &tempSetParticles.front());
                        particleSubSet->addAttribute("REALTIME", realtime.c_str());
                        particleSubSets.push_back(particleSubSet);
                    }
                    for (int nPort = 0; nPort < num_ports; ++nPort)
                    {
                        index_t portchoice = particleDataChoice[nPort]->getValue();
                        if (portchoice > 0 && portchoice <= m_case.particleFields.size()+1)
                        {
                            //std::cerr << std::time(0) << " Read data for timestep: " << t << std::endl;
                            std::string portSubSetName = particleDataPorts[nPort]->getObjName();
                            portSubSetName += s.str();
                            particlePortSubSet = new coDoSet(portSubSetName, int(tempSetParticlesPort[nPort].size()), &tempSetParticlesPort[nPort].front());
                            particlePortSubSet->addAttribute("REALTIME", realtime.c_str());
                            particlePortSubSets[nPort].push_back(particlePortSubSet);
                        }
                    }
                }
                ++timestep;
            }
            counter++;
        }
    }
    
    if (meshParam->getValue())
    {
        std::string meshSetName = meshOutPort->getObjName();
        meshSet = new coDoSet(meshSetName, int(meshSubSets.size()), &meshSubSets.front());

        std::stringstream sMesh;
        sMesh << "0-" << meshSubSets.size();
        if(meshSubSets.size()>1)
        meshSet->addAttribute("TIMESTEP", sMesh.str().c_str());
        meshSubSets.clear();
        meshOutPort->setCurrentObject(meshSet);
    }
    if (boundaryParam->getValue())
    {
        std::string boundSetName = boundaryOutPort->getObjName();
        boundarySet = new coDoSet(boundSetName, int(boundarySubSets.size()), &boundarySubSets.front());
        std::stringstream sBoundary;
        sBoundary << "0-" << boundarySubSets.size();
        if (boundarySubSets.size() > 1)
        boundarySet->addAttribute("TIMESTEP", sBoundary.str().c_str());
        boundarySubSets.clear();
        boundaryOutPort->setCurrentObject(boundarySet);
    }
    if (m_case.hasParticles && particleParam->getValue())
    {
        std::string particleSetName = particleOutPort->getObjName();
        particleSet = new coDoSet(particleSetName, int(particleSubSets.size()), &particleSubSets.front());
        std::stringstream s;
        s << "0-" << boundarySubSets.size();
        if (boundarySubSets.size() > 1)
        particleSet->addAttribute("TIMESTEP", s.str().c_str());
        particleSubSets.clear();
        particleOutPort->setCurrentObject(particleSet);
    }
    for (int nPort = 0; nPort < num_ports; ++nPort)
    {
        if (portChoice[nPort]->getValue() > 0)
        {
            std::string portSetName = outPorts[nPort]->getObjName();
            coDoSet *portSet = new coDoSet(portSetName, int(portSubSets[nPort].size()), &portSubSets[nPort].front());
            if (m_case.timedirs.size() > 1)
            {
                std::stringstream s;
                s << "0-" << portSubSets[nPort].size();
                portSet->addAttribute("TIMESTEP", s.str().c_str());
            }
            outPorts[nPort]->setCurrentObject(portSet);
        }
        portSubSets[nPort].clear();
    }
    for (int nPort = 0; nPort < num_boundary_data_ports; ++nPort)
    {
        if (boundaryDataChoice[nPort]->getValue() > 0)
        {
            std::string boundPortSetName = boundaryDataPorts[nPort]->getObjName();
            coDoSet *boundPortSet = new coDoSet(boundPortSetName, int(boundPortSubSets[nPort].size()), &boundPortSubSets[nPort].front());
            if (m_case.timedirs.size() > 1)
            {
                std::stringstream s;
                s << "0-" << boundPortSubSets[nPort].size();
                boundPortSet->addAttribute("TIMESTEP", s.str().c_str());
            }
            boundaryDataPorts[nPort]->setCurrentObject(boundPortSet);
        }
        boundPortSubSets[nPort].clear();
    }
    for (int nPort = 0; nPort < num_ports; ++nPort)
    {
        if (particleDataChoice[nPort]->getValue() > 0)
        {
            std::string particlePortSetName = particleDataPorts[nPort]->getObjName();
            coDoSet *portSet = new coDoSet(particlePortSetName, int(particlePortSubSets[nPort].size()), &particlePortSubSets[nPort].front());
            if (m_case.timedirs.size() > 1)
            {
                std::stringstream s;
                s << "0-" << particlePortSubSets[nPort].size();
                portSet->addAttribute("TIMESTEP", s.str().c_str());
            }
            particleDataPorts[nPort]->setCurrentObject(portSet);
        }
        particlePortSubSets[nPort].clear();
    }


coModule::sendInfo("ReadFOAM complete.");
std::cerr << "ReadFOAM finished." << std::endl;

return CONTINUE_PIPELINE;
}

bool ReadFOAM::vectorsAreFilled()
{
    bool filled = true;
    for (int i = 0; i < lastDataPortSelection.size(); ++i)
    {
        if (lastDataPortSelection[i].empty())
        {
            filled = false;
        }
    }
    for (int i = 0; i < lastBoundaryPortSelection.size(); ++i)
    {
        if (lastBoundaryPortSelection[i].empty())
        {
            filled = false;
        }
    }
    for (int i=0; i<lastParticlePortSelection.size(); ++i)
    {
        if (lastParticlePortSelection[i].empty())
        {
            filled = false;
        }
    }
    return filled;
}

#ifndef COV_READFOAM_LIB
MODULE_MAIN(IO, ReadFOAM)
#endif
