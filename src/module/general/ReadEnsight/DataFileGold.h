/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS    DataFileGold
//
// Description: Abstraction of a Ensight Gold ASCII Data File
//
// Initial version: 07.04.2003
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2002/2003 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Changes:
//

#ifndef DATAFILEGOLD_H
#define DATAFILEGOLD_H

#include "EnFile.h"
#include "GeoFileAsc.h"

class DataFileGold : public EnFile
{
public:
    /// default CONSTRUCTOR
    DataFileGold(ReadEnsight *mod, const string &name, const unsigned int &dim, const unsigned int &numVals);

    void read(dimType dim, coDistributedObject **outObjects, const string &baseName, int &timeStep, int numTimeSteps);

    void readCells(dimType dim, coDistributedObject **outObjects, const string &baseName, int &timeStep, int numTimeSteps);

    /// DESTRUCTOR
    ~DataFileGold();

private:
    unsigned int lineCnt_; // actual linecount
    unsigned int numVals_; // number of values
    unsigned int *indexMap_; // may contain indexMap
    unsigned int actPartIndex_;
};

#endif
