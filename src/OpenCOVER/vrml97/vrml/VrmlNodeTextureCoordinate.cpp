/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

//
//  Vrml 97 library
//  Copyright (C) 1998 Chris Morley
//
//  %W% %G%
//  VrmlNodeTextureCoordinate.cpp

#include "VrmlNodeTextureCoordinate.h"
#include "VrmlNodeType.h"

using namespace vrml;

void VrmlNodeTextureCoordinate::initFields(VrmlNodeTextureCoordinate *node, VrmlNodeType *t)
{
    initFieldsHelper(node, t, exposedField("point", node->d_point));
}

const char *VrmlNodeTextureCoordinate::typeName() 
{
    return "TextureCoordinate";
}

VrmlNodeTextureCoordinate::VrmlNodeTextureCoordinate(VrmlScene *scene)
    : VrmlNode(scene, typeName())
{
}
