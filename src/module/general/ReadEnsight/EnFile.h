/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

//-*-Mode: C++;-*-
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS  EnFile
// CLASS  DataCont
//
// Description: Ensight file representation ( base class)
//
// Initial version: 01.06.2002 by RM
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2001 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Changes:
//

#ifndef ENFILE_H
#define ENFILE_H

#include <util/coviseCompat.h>

#include "EnElement.h"
#include "EnPart.h"
#include "CaseFile.h"
class ReadEnsight;

namespace covise
{
class coModule;
}

// primitive data container
// be aware DataCont is only an assembly of pointers and numbers
// USE THIS CLASS WITH CARE !!! it is dangerous !!!
class DataCont
{
public:
    DataCont();

    virtual ~DataCont();

    void setNumCoord(const uint64_t &n)
    {
        nCoord_ = n;
    };
    void setNumElem(const uint64_t &n)
    {
        nElem_ = n;
    };
    void setNumConn(const uint64_t &n)
    {
        nConn_ = n;
    };

    uint64_t getNumCoord() const
    {
        return nCoord_;
    };
    uint64_t getNumElem() const
    {
        return nElem_;
    };
    uint64_t getNumConn() const
    {
        return nConn_;
    };

    float *x;
    float *y;
    float *z;

    unsigned int *el;
    unsigned int *cl;
    unsigned int *tl;

    void cleanAll();

private:
    uint64_t nCoord_;
    uint64_t nElem_;
    uint64_t nConn_;
};

class InvalidWordException
{
public:
    InvalidWordException(const string &type);
    string what();

private:
    string type_;
};

//
// base class for Ensight geometry files
// provide general methods for reading geometry files
//
class EnFile
{
public:
    typedef enum
    {
        DIM1D,
        DIM2D,
        DIM3D,
        GEOMETRY
    } dimType;
    enum
    {
        OFF,
        GIVEN,
        ASSIGN,
        EN_IGNORE
    };
    typedef enum
    {
        CBIN,
        FBIN,
        NOBIN,
        UNKNOWN
    } BinType;

    EnFile(ReadEnsight *mod, const BinType &binType = UNKNOWN);

    EnFile(ReadEnsight *mod, const string &name, const BinType &binType = UNKNOWN);

    EnFile(ReadEnsight *mod, const string &name, const unsigned int &dim, const BinType &binType = UNKNOWN);

    bool isOpen();

    // check for binary file
    BinType binType();

    // read the file
    virtual void read(dimType dim, coDistributedObject **outObjects, const string &baseName, int &timeStep, int numTimeSteps) { fprintf(stderr, "oops, please implement this\n"); };
    // read the file
    virtual void read(dimType dim, coDistributedObject **outObjects2d, coDistributedObject **outObjects3d, const string &actObjNm2d, const string &actObjNm3d, int &timeStep, int numTimeSteps) { fprintf(stderr, "oops, please implement this\n"); };
    // read cell based data
    virtual void readCells(dimType dim, coDistributedObject **outObjects, const string &baseName, int &timeStep, int numTimeSteps) { fprintf(stderr, "oops, please implement this\n"); };

    // Return data in data container.
    // this function is dangerous as only adresses are returned
    // copying the data would be to expensive
    DataCont getDataCont() const;

    // Set the master part list. This is the list of all part in the geometry file
    // or the geo. file for the first timestep. This means we must still check the
    // length of the connection table

    virtual void setPartList(PartList *p);

    virtual ~EnFile();

    virtual void parseForParts(){};

    // fill data container with data values ( only for the case of cell based data)
    virtual void buildParts(const bool &isPerVert = false);

    // use this function to create a geometry Ensight file representation
    // each Ensight geometry file has a own fctory to create associated
    // data files
    // you will have to change this method each time you enter a new type of
    // Ensight Geometry
    static EnFile *createGeometryFile(ReadEnsight *mod, const CaseFile &c, const string &filename);
    void createGeoOutObj(dimType dim, coDistributedObject **outObjects2d, coDistributedObject **outObjects3d, const string &actObjNm2d, const string &actObjNm3d, int &timeStep);
    void createDataOutObj(dimType dim, coDistributedObject **outObjects, const string &baseName, int &timeStep, int numTimeSteps, bool perVertex=true);
 
    void setActiveAlloc(const bool &b);

    void setDataByteSwap(const bool &v);
    void setIncludePolyeder(const bool &b);
    virtual coDistributedObject *getDataObject(std::string)
    {
        return NULL;
    };

    bool fileMayBeCorrupt_;

protected:
    // functions used for BINARY input
    virtual string getStr();

    // skip n ints
    void skipInt(const uint64_t &n);

    // skip n floats
    void skipFloat(const uint64_t &n);

    // get integer
    virtual int getInt();
    virtual unsigned int getuInt();
    int getIntRaw();
    unsigned int getuIntRaw();

    // get integer array
    virtual int *getIntArr(const uint64_t &n, int *iarr = NULL);

    // get unsigned integer array
    virtual void getuIntArr(const uint64_t &n, unsigned int *iarr = NULL);

    // get float array
    virtual float *getFloatArr(const uint64_t &n, float *farr = NULL);

    // find a part by its part number
    virtual EnPart *findPart(const unsigned int &partNum) const;

    // find a part by its part number in the master part list
    virtual EnPart findMasterPart(const unsigned int &partNum) const;

    virtual void resetPart(const unsigned int &partNum, EnPart *p);

    // send a list of all parts to covise info
    void sendPartsToInfo();

    string className_;

    bool isOpen_;

    FILE *in_;

    unsigned int nodeId_;

    unsigned int elementId_;

    DataCont dc_; // container for read data

    BinType binType_;

    bool byteSwap_;

    PartList *partList_;

    unsigned int dim_;

    bool activeAlloc_;

    bool dataByteSwap_;

    bool includePolyeder_;
    // pointer to module for sending ui messages
    ReadEnsight *ens;

private:
    string name_;

    void getIntArrHelper(const uint64_t &n, unsigned int *iarr = NULL);
};
#endif
