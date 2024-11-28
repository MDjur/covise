/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

//-*-Mode: C++;-*-
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS    EnGoldGeoBIN
//
// Description: Abstraction of Ensight Gold geometry Files
//
// Initial version: 08.08.2003
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2002 / 2003 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Changes:
//

#ifndef ENGOLDGEOBIN_H
#define ENGOLDGEOBIN_H

#include "EnFile.h"

class EnGoldGeoBIN : public EnFile
{
public:
    /// default CONSTRUCTOR
    EnGoldGeoBIN(ReadEnsight *mod);

    // creates file-rep. and opens the file
    EnGoldGeoBIN(ReadEnsight *mod, const string &name, EnFile::BinType binType = EnFile::CBIN);

    /// read the file
    void read(dimType dim, coDistributedObject **outObjects2d, coDistributedObject **outObjects3d, const string &actObjNm2d, const string &actObjNm3d, int &timeStep, int numTimeSteps);

    // get part info
    void parseForParts();

    /// DESTRUCTOR
    ~EnGoldGeoBIN();

private:
    int allocateMemory();

    // read header
    int readHeader();

    // read Ensight part information
    int readPart(EnPart &actPart);

    // read part connectivities (ENSIGHT Gold only)
    int readPartConn(EnPart &actPart);

    // read bounding box (ENSIGHT Gold)
    int readBB();

    // skip part
    int skipPart();

    // redundant find a slution must go into base-class
    void fillIndexMap(const unsigned int &i, const unsigned int &natIdx);

    unsigned int lineCnt_; // actual linecount
    unsigned int numCoords_; // number of coordinates
    unsigned int *indexMap_; // index map array if node id: GIVEN
    unsigned int maxIndex_; // max possible  index of indexmap
    unsigned int lastNc_;
    unsigned int globalCoordIndexOffset_;
    unsigned int actPartNumber_;
    unsigned int currElementIdx_;
    unsigned int currCornerIdx_;

    bool allocated_;

    bool partFound;
};
#endif
