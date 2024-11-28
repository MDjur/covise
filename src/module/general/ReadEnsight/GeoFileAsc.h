/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

//-*-Mode: C++;-*-
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CLASS  En6GeoASC
//
// Description: Ensight 6 geo-file representation
//
// Initial version: 01.06.2002 by RM
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2001 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Changes:
//

#ifndef GEOFILEASC_H
#define GEOFILEASC_H

#include <util/coviseCompat.h>
#ifdef __sgi
using namespace std;
#endif

#include <do/coDoPoints.h>
#include "EnFile.h"
#include "EnElement.h"
#include "EnPart.h"

// helper strip off spaces
string strip(const string &str);

const unsigned int lineLen(250);

//
// read Ensight 6 geometry files
//
class En6GeoASC : public EnFile
{
public:
    /// default CONSTRUCTOR
    En6GeoASC(ReadEnsight *mod);

    // creates file-rep. and opens the file
    En6GeoASC(ReadEnsight *mod, const string &name);

    // read the file (Ensight 6)
    void read(dimType dim, coDistributedObject **outObjects2d, coDistributedObject **outObjects3d, const string &actObjNm2d, const string &actObjNm3d, int &timeStep, int numTimeSteps);

    // create a list of Parts
    virtual void parseForParts();

    /// DESTRUCTOR
    virtual ~En6GeoASC();

    // helper converts char buf containing num ints of length int_leng to int-array arr
    // -> should better go to EnFile
    static void atoiArr(const unsigned int &int_leng, char *buf, unsigned int *arr, const unsigned int &num);

private:
    // read header (ENSIGHT 6, Gold)
    int readHeader();

    // read coordinates (ENSIGHT 6)
    int readCoords();

    // read connectivity (ENSIGHT 6)
    int readConn();

    // read part information (ENSIGHT Gold only)
    int readPart();

    // read part connectivities (ENSIGHT Gold only)
    int readPartConn();

    // enter value i to index map (incl realloc)
    void fillIndexMap(const unsigned int &i, const unsigned int &natIdx);

    unsigned int lineCnt_; // actual linecount
    unsigned int numCoords_; // number of coordinates
    unsigned int *indexMap_; // index map array if node id: GIVEN
    unsigned int maxIndex_; // max possible  index of indexmap
    unsigned int lastNc_;

    //vector<EnPart> parts_; // contains all parts of the current geometry
};
#endif
