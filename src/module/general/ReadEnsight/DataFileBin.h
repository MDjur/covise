/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS    DataFileBin
//
// Description: Read binary data files (Ensight 6)
//
// Initial version: RM 17.07.002
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2002 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Changes:
//

#ifndef DATAFILEBIN_H
#define DATAFILEBIN_H

#include "EnFile.h"
#include "ReadEnsight.h"

class DataFileBin : public EnFile
{
public:
    /// default CONSTRUCTOR
    DataFileBin(ReadEnsight *mod);
    DataFileBin(ReadEnsight *mod,
                const string &name,
                const unsigned int &dim,
                const unsigned int &numVals,
                const EnFile::BinType &binType = CBIN);

    void read(dimType dim, coDistributedObject **outObjects, const string &baseName, int &timeStep, int numTimeSteps);

    void readCells(dimType dim, coDistributedObject **outObjects, const string &baseName, int &timeStep, int numTimeSteps);

    virtual coDistributedObject *getDataObject(std::string s);

private:
    unsigned int dim_;
    unsigned int lineCnt_; // actual linecount
    unsigned int numVals_; // number of values
    unsigned int *indexMap_; // may contain indexMap
};
#endif
