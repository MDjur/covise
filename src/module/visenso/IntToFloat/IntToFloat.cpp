/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */


#include "IntToFloat.h"

#include <do/coDoData.h>
#include <do/coDoIntArr.h>

IntToFloat::IntToFloat(int argc, char *argv[])
    : coSimpleModule(argc, argv, "Convert IntArray to Float")
{
    p_input = addInputPort("Input", "IntArr|Byte", "input");
    p_input->setRequired(1);
    p_output = addOutputPort("Output", "Float", "output");
}

int IntToFloat::compute(const char *)
{

    const coDistributedObject *dobj = p_input->getCurrentObject();
    if (!dobj)
        return FAIL;
    const coDoIntArr *intarr = dynamic_cast<const coDoIntArr *>(dobj);
    const coDoByte *bytearr = dynamic_cast<const coDoByte *>(dobj);

    int numPoints=0;
    if (intarr)
    	numPoints = intarr->getSize() - 1;
    if(bytearr)
   	numPoints = bytearr->getNumPoints();
    const char *outName = p_output->getObjName();
    coDoFloat *outObj = new coDoFloat(outName, numPoints);
    outObj->copyAllAttributes(dobj);
    // CREATE

    // CALCULATE
    float *outPtr;
    outObj->getAddress(&outPtr);

    if(intarr) {
    	int *inPtr;
   	intarr->getAddress(&inPtr);
    	for (int i = 0; i < numPoints; ++i)
    	{
        	outPtr[i] = (float)inPtr[i];
    	}
    }
    if(bytearr) {
    	unsigned char *inPtr;
   	bytearr->getAddress(&inPtr);
    	for (int i = 0; i < numPoints; ++i)
    	{
        	outPtr[i] = (float)(inPtr[i]/255.0);
    	}
    }

    p_output->setCurrentObject(outObj);

    return SUCCESS;
}

MODULE_MAIN(Converter, IntToFloat)
