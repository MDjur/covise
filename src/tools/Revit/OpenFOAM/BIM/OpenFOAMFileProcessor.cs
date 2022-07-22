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
        private Dictionary<string, string> fileData;
        private Dictionary<string, Dictionary<string, string>> fileContents;

        public OpenFOAMFileProcessor(in string filename)
        {
            this.filename = filename;
            this.fileData = new Dictionary<string, string>();
            this.fileContents = new Dictionary<string, Dictionary<string, string>>();
            try //read file (with automatic file opening and closing as part of utilized method)
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
                        lines[idx] = lines[idx].Substring(0, lines[idx].IndexOf("/*")) + lines[idx].Substring(lines[idx].IndexOf("*/") + 2);
                    else //remove all block comment text across lines but preserve any writing in the first/last line before/after the block comment
                    {
                        lines[idx] = lines[idx].Substring(0, lines[idx].IndexOf("/*")); //remove the block comment text from the line but keep everything before it
                        while (++idx < lines.Length && !lines[idx].Contains("*/"))
                            lines[idx] = String.Empty; //remove comment text and advance to next line
                        if (idx >= lines.Length)
                            throw new OpenFOAMFileFormatException("Multi-line comment is not closed properly in config file " + filename + ".");
                        else 
                            lines[idx] = lines[idx].Substring(lines[idx].IndexOf("*/") + 2); //remove the block comment text from the line but keep everything after it
                    }
                }

                //remove leading and trailing whitespace from every line
                lines[idx] = lines[idx].Trim();
            }
        }

        private void extractFileData()
        {
            int lineNum = 0;
            while (lineNum < lines.Length)
            {
                if (String.IsNullOrEmpty(lines[lineNum])) //skip any blank lines
                    lineNum++; 
                else if (lineNum + 2 >= lines.Length) //this is a syntax error
                    throw new OpenFOAMFileFormatException("Improper or incomplete data contained in config file " + filename + " beginning at line " + lineNum + ".");
                else if (lines[lineNum].ToLower().Equals("foamfile")) //this section contains the file data (lowercase to ensure correct text identification)
                    lineNum = processFileData(lineNum);
                else //this section contains the file contents
                    lineNum = processDictionaryEntry(lineNum);
            }
        }

        private int processFileData(int lineNum)
        {
            lineNum++; //skip file data header
            while (lineNum < lines.Length && String.IsNullOrEmpty(lines[lineNum]))
                lineNum++; //skip any blank lines
            if (lineNum >= lines.Length || !lines[lineNum].Equals("{"))
                throw new OpenFOAMFileFormatException("Improper file information syntax in config file " + filename + ".");
            else
            {
                lineNum++; //skip opening brace
                while (lineNum < lines.Length && !lines[lineNum].Equals("}"))
                {
                    if (String.IsNullOrEmpty(lines[lineNum]))
                        continue; //skip any blank lines
                    string[] currLine = lines[lineNum].Split('\t');
                    string currKey = String.Empty;
                    string currVal = String.Empty;
                    Boolean missingSemicolon = true;
                    foreach (string section in currLine)
                    {
                        section.Trim(); //remove remaining whitespace on sections
                        if (String.IsNullOrEmpty(section))
                            continue; //skip any blank sections which only contained whitespace characters
                        else if (String.IsNullOrEmpty(currKey))
                            currKey = section; //first section of text in the line is the key
                        else if (String.IsNullOrEmpty(currVal))
                            if (currVal.EndsWith(";")) //parsing information with attached semicolon
                            {
                                currVal = section.Substring(0, currVal.Length - 1); //second section of text in the line is the value
                                missingSemicolon = false;
                            }
                            else //parsing information without a semicolon
                                currVal = section;
                        else if (missingSemicolon && section.Equals(";"))
                            missingSemicolon = false;
                        else //additional text in the line indicates a syntax error
                            throw new OpenFOAMFileFormatException("Improper file information syntax in config file " + filename + " at line number " + lineNum + ".");
                    }
                    if (String.IsNullOrEmpty(currKey) || String.IsNullOrEmpty(currVal) || missingSemicolon)
                        throw new OpenFOAMFileFormatException("Improper file information syntax in config file " + filename + " at line number " + lineNum + ".");
                    fileData.Add(currKey.ToLower(), currVal); //key in lowercase to ensure compatibility with getter methods for specific information
                    lineNum++; //current line has been processed, so advance to the next line and continue
                }
                if (lineNum >= lines.Length)
                    throw new OpenFOAMFileFormatException("Improper file information syntax in config file " + filename + ".");
                else if (lines[lineNum].Equals("}"))
                    lineNum++; //skip closing brace
            }
            return lineNum;
        }

        private int processDictionaryEntry(int lineNum)
        {
            string key = lines[lineNum++]; 
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
                    string currKey = String.Empty;
                    string currVal = String.Empty;
                    Boolean missingSemicolon = true;
                    foreach (string section in currLine)
                    {
                        section.Trim(); //remove remaining whitespace on sections
                        if (String.IsNullOrEmpty(section))
                            continue; //skip any blank sections which only contained whitespace characters
                        else if (String.IsNullOrEmpty(currKey))
                            currKey = section; //first section of text in the line is the key
                        else if (String.IsNullOrEmpty(currVal))
                            if (currVal.EndsWith(";")) //parsing information with attached semicolon
                            {
                                currVal = section.Substring(0, currVal.Length - 1); //second section of text in the line is the value
                                missingSemicolon = false;
                            }
                            else //parsing information without a semicolon
                                currVal = section;
                        else if (missingSemicolon && section.Equals(";"))
                            missingSemicolon = false;
                        else //additional text in the line indicates a syntax error
                            throw new OpenFOAMFileFormatException("Improper dictionary entry syntax in config file " + filename + " at line number " + lineNum + ".");
                    }
                    if (String.IsNullOrEmpty(currKey) || String.IsNullOrEmpty(currVal) || missingSemicolon)
                        throw new OpenFOAMFileFormatException("Improper dictionary entry syntax in config file " + filename + " at line number " + lineNum + ".");
                    dict.Add(currKey, currVal);
                    lineNum++; //current line has been processed, so advance to the next line and continue
                }
                if (lineNum >= lines.Length)
                    throw new OpenFOAMFileFormatException("Improper dictionary syntax in config file " + filename + ".");
                else if (lines[lineNum].Equals("}"))
                    lineNum++; //skip closing brace
                fileContents.Add(key, dict);
            }
            return lineNum;
        }

        public string getFileVersion()
        {
            if (fileData.ContainsKey("version"))
                return fileData["version"];
            else
                throw new OpenFOAMFileFormatException("Background information not initialized properly for config file " + filename + ".");
        }

        public string getFileFormat()
        {
            if (fileData.ContainsKey("format"))
                return fileData["format"];
            else
                throw new OpenFOAMFileFormatException("Background information not initialized properly for config file " + filename + ".");
        }

        public string getFileClass()
        {
            if (fileData.ContainsKey("class"))
                return fileData["class"];
            else
                throw new OpenFOAMFileFormatException("Background information not initialized properly for config file " + filename + ".");
        }

        public string getFileLocation()
        {
            if (fileData.ContainsKey("location"))
                return fileData["location"];
            else
                throw new OpenFOAMFileFormatException("Background information not initialized properly for config file " + filename + ".");
        }

        public string getFileObject()
        {
            if (fileData.ContainsKey("object"))
                return fileData["object"];
            else
                throw new OpenFOAMFileFormatException("Background information not initialized properly for config file " + filename + ".");
        }

        public Dictionary<string, Dictionary<string, string>> getFileContents()
        {
            return fileContents;
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
