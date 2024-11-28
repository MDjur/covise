/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef _READ_OBJ_H
#define _READ_OBJ_H
/**************************************************************************\ 
 **                                                   	      (C)1999 RUS **
 **                                                                        **
 ** Description: Reader for Wavefront OBJ Format	                          **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author: D. Rainer		                                              **
 **                                                                        **
 ** History:  								                              **
 ** 01-September-99 v1					       		                      **
 **                                                                        **
 **                                                                        **
\**************************************************************************/

#include <api/coModule.h>
#include <filesystem>
#include <vector>
#include <filesystem>
using namespace covise;

class ReadObj : public coModule
{

public:
    ReadObj(int argc, char *argv[]);

    //  member functions
    int compute(const char *);

private:
    FILE *openFile(const char *filename);
    void readObjFile();
    void readMtlFile();
    int makePackedColor(float r, float g, float b, float a);
    int getCurrentColor(const char *mtlName);

    //  member data
    coOutputPort *polyOut, *colorOut, *normalOut, *textureOut;
    coFileBrowserParam *objFileBrowser, *mtlFileBrowser;
    FILE *objFp, *mtlFp;
    int numMtls; // no of materials in the mtl file
    int *pcList; // list of packed colors from the mtl file
    int currentColor; // current color in packed format
    constexpr static int MAX_PATH_LEN=8192;
    typedef char PATH[MAX_PATH_LEN];
    PATH *mapKdList; // list KD map paths
    typedef struct { float x, y; } TexCoord;
    typedef std::vector<TexCoord> TexCoordList;
    std::vector<TexCoordList> tcList; // list of tex coords, per mesh
    typedef char mtlNameType[100];
    mtlNameType *mtlNameList; // list of material names in the mtl file
    std::filesystem::path basePath; // path where to search for textures etc.
};
#endif
