/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

using System;
using System.IO;

namespace OpenFOAMInterface.BIM
{
    public class OpenFOAMFileProcessor
    {
        string[] lines;

        public OpenFOAMFileProcessor(in string filename)
        {
            try //read file
            {
                lines = File.readAllLines(filename);
            }
            catch (Exception e) //error reading file
            {
                OpenFOAMDialogManager.ShowDialogException(e);
                return;
            }

            removeComments();
            extractFileData();
        }

        private removeComments()
        {
            for (int idx = 0; idx < lines.Length; idx++)
            {
                //handle single line comments
                if (lines[idx].Contains("//"))
                    lines[idx] = lines[idx].Substring(0, lines[idx].IndexOf("//");

                //handle block (multi-line) comments
                if (lines[idx].Contains("/*"))
                {
                    if (lines[idx].Contains("*/")) //remove all block comment text where block comment is in a single line
                    {
                        lines[idx] = lines[idx].Substring(0, lines[idx].IndexOf("/*")) + lines[idx].Substring(lines[idx].IndexOf("*/")+2);
                    } else
                    {
                        //remove the start of the block comment but preserve any text in the line before it begins
                        if (idx + 1 >= lines.Length)
                            throw new OpenFOAMFileFormatException("Multi-line comment is not closed properly."); //TODO: finish
                        lines[idx++] = lines[idx].Substring(0, lines[idx].IndexOf("/*")); //move to the next line (as this line has already been treated to remove the comment
                        while (!lines[idx].Contains("*/"))
                            lines[idx++] = "";
                        lines[idx] = lines[idx].Substring(lines[idx].IndexOf("*/") + 2);
                    }
                }
            }
        }

        private extractFileData()
        {
            //TODO: implement
        }


    }

    public class OpenFOAMFileFormatException : Exception
    {
        public OpenFOAMFileFormatException(string message)
        {
            //TODO: Check Ideal Implementation 
        }
    }
}
