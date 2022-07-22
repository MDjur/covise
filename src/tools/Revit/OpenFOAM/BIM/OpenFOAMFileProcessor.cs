/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

using System;
using System.IO;
using System.Collections.Generic;
using System.Windows.Forms;

namespace OpenFOAMInterface.BIM
{
    public class OpenFOAMFileProcessor
    {
        private string filename;
        private string[] lines;
        private Dictionary<string, Dictionary<string, string>> fileData;

        public OpenFOAMFileProcessor(in string filename)
        {
            this.filename = filename;
            this.fileData = new Dictionary<string, Dictionary<string, string>>();
            try //read file
            {
                this.lines = File.ReadAllLines(filename);
            }
            catch (Exception e) //error reading file
            {
                OpenFOAMDialogManager.ShowDialogException(e);
                return;
            }

            isolateFileData();
            extractFileData();
        }

        private void isolateFileData()
        {
            for (int idx = 0; idx < lines.Length; idx++)
            {
                //remove single line comments
                if (lines[idx].Contains("//"))
                    lines[idx] = lines[idx].Substring(0, lines[idx].IndexOf("//"));

                //remove block (multi-line) comments
                if (lines[idx].Contains("/*"))
                {
                    if (lines[idx].Contains("*/")) //remove all block comment text where block comment is in a single line
                    {
                        lines[idx] = lines[idx].Substring(0, lines[idx].IndexOf("/*")) + lines[idx].Substring(lines[idx].IndexOf("*/") + 2);
                    } else
                    {
                        //remove the start of the block comment but preserve any text in the line before it begins
                        if (idx + 1 >= lines.Length)
                            throw new OpenFOAMFileFormatException("Multi-line comment is not closed properly in config file " + filename + ".");
                        lines[idx++] = lines[idx].Substring(0, lines[idx].IndexOf("/*")); //move to the next line (as this line has already been treated to remove the comment
                        while (!lines[idx].Contains("*/"))
                            lines[idx++] = "";
                        lines[idx] = lines[idx].Substring(lines[idx].IndexOf("*/") + 2);
                    }
                }

                //remove leading and trailing whitespace from every line
                lines[idx] = lines[idx].Trim();
            }
        }

        private void extractFileData()
        {
            //TODO: handle FoamFile information
            int lineNum = 0;
            while (lineNum < lines.Length)
            {
                if (String.IsNullOrEmpty(lines[lineNum]))
                    lineNum++; //skip any blank lines
                else if (lineNum + 2 >= lines.Length)
                    throw new OpenFOAMFileFormatException("Improper or incomplete data contained in config file " + filename + " beginning at line " + lineNum + ".");
                else
                    lineNum = processDictionaryEntry(lineNum);
            }
        }

        private int processDictionaryEntry(int lineNum)
        {
            string key = lines[lineNum++]; //identify the key and advance to determine value type
            while (lineNum < lines.Length && String.IsNullOrEmpty(lines[lineNum]))
                lineNum++; //skip any blank lines
            if (lineNum >= lines.Length || !lines[lineNum].Equals("{"))
                throw new OpenFOAMFileFormatException("Improper dictionary syntax in config file " + filename + ".");
            else
            {
                lineNum++; //skip opening brace
                Dictionary<string, string> dict = new Dictionary<string, string>();
                while (lineNum < lines.Length && !lines[lineNum].Equals("}")) {
                    if (String.IsNullOrEmpty(lines[lineNum]))
                        continue; //skip any blank lines
                    string[] currLine = lines[lineNum].Split('\t');
                    string currKey = "";
                    string currVal = "";
                    foreach (string section in currLine)
                    {
                        section.Trim(); //remove remaining whitespace on sections
                        if (String.IsNullOrEmpty(section))
                            continue;
                        else if (String.IsNullOrEmpty(currKey))
                            currKey = section;
                        else if (String.IsNullOrEmpty(currVal))
                            currVal = section;
                        else
                            throw new OpenFOAMFileFormatException("Improper dictionary entry syntax in config file " + filename + " at line number " + lineNum + ".");
                    }
                    if (String.IsNullOrEmpty(currKey) || String.IsNullOrEmpty(currVal))
                        throw new OpenFOAMFileFormatException("Improper dictionary entry syntax in config file " + filename + " at line number " + lineNum + ".");
                    dict.Add(currKey, currVal);
                    lineNum++; //current line has been processed, so advance to the next line and continue
                }
                if (lineNum >= lines.Length)
                    throw new OpenFOAMFileFormatException("Improper dictionary syntax in config file " + filename + ".");
                else if (lines[lineNum].Equals("}"))
                    lineNum++; //skip closing brace
                fileData.Add(key, dict);
            }
            return lineNum;
        }


    }

    public class OpenFOAMFileFormatException : Exception
    {
        public OpenFOAMFileFormatException(string message)
        {
            MessageBox.Show(message, OpenFOAMInterfaceResource.MESSAGE_BOX_TITLE, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }
    }
}
