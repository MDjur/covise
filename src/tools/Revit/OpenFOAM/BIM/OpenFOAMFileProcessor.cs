/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

using System;

namespace OpenFOAMInterface.BIM

public class OpenFOAMFileProcessor
{
	public OpenFOAMFileProcessor(in string filename)
	{
		try
        {
            string[] lines = File.readAllLines(filename);
        } catch (Exception e)
        {
            OpenFOAMDialogManager.ShowDialogException(e);
            return;
        }
	}
}
