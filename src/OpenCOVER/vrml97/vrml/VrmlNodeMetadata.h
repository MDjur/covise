/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

//
//  Vrml 97 library
//  Copyright (C) 1998 Chris Morley
//
//  %W% %G%
//  VrmlNodeMetadata.h

#ifndef _VRMLNODEMETADATA_
#define _VRMLNODEMETADATA_

#include "VrmlSFString.h"
#include "VrmlMFString.h"

#include "VrmlNode.h"

namespace vrml
{

class VRMLEXPORT VrmlNodeMetadata : public VrmlNode
{

public:
    // Define the built in VrmlNodeType:: "Metadata"
    static void initFields(VrmlNodeMetadata *node, VrmlNodeType *t);
    static const char *typeName();

    VrmlNodeMetadata(VrmlScene *, const std::string &n = typeName());

private:
    VrmlSFString d_name;
    VrmlSFString d_reference;
};
}
#endif //_VRMLNODEMETADATA_
