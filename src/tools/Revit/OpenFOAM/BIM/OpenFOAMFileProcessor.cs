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

    /// <summary>
    /// This class is designed to process config files written in the OpenFOAM syntax style and store the filename, file text (currently inaccessible outside of the class),
    /// file background data, and file contents (the last two of which are acheived through file processing in the constructor) in order to use the information from the file 
    /// to set base and default values.
    /// </summary>
    public class OpenFOAMFileProcessor
    {
        private string filename; //name of the file being processed by the instance of the class
        private string[] lines; //contents of the file broken into an array by occurences of newline characters
        private Dictionary<string, string> fileData; //background information about the file itself
        private Dictionary<string, Dictionary<string, string>> fileContents; //dictionary information stored in file

        /// <summary>
        /// Constructor that creates a new file processor instance for the specific file provided (by filename) and parses the file to populate all class fields
        /// </summary>
        /// <param name="filename">String containing the name of the file to be processed</param>
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

        /// <summary>
        /// This is a private helper method for the constructor which performs preprocessing on the lines in the file to remove comments and leading and trailing whitespace
        /// </summary>
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

        /// <summary>
        /// This is a private helper method for the constructor which parses data from the input file (after it has undergone preprocessing in isolateFileData() 
        /// in order to populate both the fileData and fileContents fields.
        /// </summary>
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

        /// <summary>
        /// This is a private helper method for the extractFileData method, which takes in line number expected to contain information about the file and processes it accordingly.
        /// </summary>
        /// <param name="lineNum">int indicating the current line that must be processed (in connection with any later lines related to it)</param>
        /// <returns>int indicating the new line number after processing all relevant information for the original line number</returns>
        /// <throws>OpenFOAMFileFormatException indicating that there is a syntax error in the file being processed if the information is not presented as expected</throws>
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

        /// <summary>
        /// This is a private helper method for the extractFileData method, which takes in line number expected to contain file contents and proceses it accordingly.
        /// </summary>
        /// <param name="lineNum">int indicating the current line that must be processed (in connection with any later lines related to it)</param>
        /// <returns>int indicating the new line number after processing all relevant information for the original line number</returns>
        /// <throws>OpenFOAMFileFormatException indicating that there is a syntax error in the file being processed if the information is not presented as expected</throws>
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

        /// <summary>
        /// This is a public getter method for the name of the file (contained within private field filename).
        /// </summary>
        /// <returns>string indicating the file name</returns>
        public string getFilename() => filename;

        /// <summary>
        /// This is a public getter method for the version of the file (contained within fileData).
        /// </summary>
        /// <returns>string indicating the file version</returns>
        /// <throws>OpenFOAMFileFormatException indicating that version was not intialized properly if the version data is missing from fileData</throws>
        public string getFileVersion()
        {
            if (fileData.ContainsKey("version"))
                return fileData["version"];
            else
                throw new OpenFOAMFileFormatException("Version information not initialized properly in config file " + filename + ".");
        }

        /// <summary>
        /// This is a public getter method for the format of the file (contained within private field fileData).
        /// </summary>
        /// <returns>string indicating the file format</returns>
        /// <throws>OpenFOAMFileFormatException indicating that format was not intialized properly if the format data is missing from fileData</throws>
        public string getFileFormat()
        {
            if (fileData.ContainsKey("format"))
                return fileData["format"];
            else
                throw new OpenFOAMFileFormatException("Format information not initialized properly in config file " + filename + ".");
        }

        /// <summary>
        /// This is a public getter method for the class of the file (contained within private field fileData).
        /// </summary>
        /// <returns>string indicating the file class</returns>
        /// <throws>OpenFOAMFileFormatException indicating that class was not intialized properly if the class data is missing from fileData</throws>
        public string getFileClass()
        {
            if (fileData.ContainsKey("class"))
                return fileData["class"];
            else
                throw new OpenFOAMFileFormatException("Class information not initialized properly in config file " + filename + ".");
        }

        /// <summary>
        /// This is a public getter method for the location of the file (contained within private field fileData).
        /// </summary>
        /// <returns>string indicating the file location</returns>
        /// <throws>OpenFOAMFileFormatException indicating that location was not intialized properly if the location data is missing from fileData</throws>
        public string getFileLocation()
        {
            if (fileData.ContainsKey("location"))
                return fileData["location"];
            else
                throw new OpenFOAMFileFormatException("Location information not initialized properly in config file " + filename + ".");
        }

        /// <summary>
        /// This is a public getter method for the object of the file (contained within private field fileData).
        /// </summary>
        /// <returns>string indicating the file object</returns>
        /// <throws>OpenFOAMFileFormatException indicating that object was not intialized properly if the object data is missing from fileData</throws>
        public string getFileObject()
        {
            if (fileData.ContainsKey("object"))
                return fileData["object"];
            else
                throw new OpenFOAMFileFormatException("Object information not initialized properly in config file " + filename + ".");
        }

        /// <summary>
        /// This is a public getter method for the contents of the file (or the entirety of the private field fileContents).
        /// </summary>
        /// <returns>Dictionary<string, Dictionary<string, string>> containing the contents of the parsed file</returns>
        public Dictionary<string, Dictionary<string, string>> getFileContents() => fileContents;
    }

    /// <summary>
    /// This is an exception class written to represent file format exceptions in an OpenFOAM config file processed by this class.  It contains four constructors 
    /// for these types of errors: one which is a basic default constructor, one which sets only the message, one which sets the message and inner exception properties, and 
    /// one which makes it serializable.
    /// </summary>
    [Serializable]
    public class OpenFOAMFileFormatException : Exception
    {
        /// <summary>
        /// This is the default constructor for the OpenFOAMFileFormatException, which simply relies on the base Excpetion constructor 
        /// </summary>
        public OpenFOAMFileFormatException() : base() 
        {
            String message = "An error occurred when processing a config file.  Please ensure all config files utilize standard OpenFOAM syntax conventions.";
            MessageBox.Show(message, OpenFOAMInterfaceResource.MESSAGE_BOX_TITLE, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        /// <summary>
        /// This is the constructor for an OpenFOAMFileFormatException with a specific error message (provided via parameter) containing information regarding the type of syntax
        /// error which triggered the excpetion in the file (potentially including a filename and/or line number).
        /// </summary>
        /// <param name="message">This is the message providing information about the syntax error that occurred</param>
        public OpenFOAMFileFormatException(string message) : base(message) 
        {
            //currently all instances of the exception utilize this constructor
            MessageBox.Show(message, OpenFOAMInterfaceResource.MESSAGE_BOX_TITLE, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        /// <summary>
        /// This is the constructor for an OpenFOAMFileFormatException with a specific error message (provided via parameter) containing information regarding the type of syntax
        /// error which triggered the excpetion in the file (potentially including a filename and/or line number) along with innerException information to pass to the base 
        /// Exception constructor.
        /// </summary>
        /// <param name="message">This is the message providing information about the syntax error that occurred</param>
        /// <param name="innerException">This is the innerException infomration regarding the current exception and how it occurred</param>
        public OpenFOAMFileFormatException(string message, Exception innerException) : base(message, innerException)
        {
            MessageBox.Show(message, OpenFOAMInterfaceResource.MESSAGE_BOX_TITLE, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        /// <summary>
        /// This is the constructor for an OpenFOAMFileFormatException which makes it serializable.
        /// NOTE: Serialization has not yet been utilized and is not implemented elsewhere in the file at this time.
        /// </summary>
        /// <param name="info">System.Runtime.Serialization.SerializationInfo necessary for serialization</param>
        /// <param name="context">System.Runtime.Serialization.StreamingContext necessary for serialization</param>
        //TODO: implement serialization should that behavior be desired
        protected OpenFOAMFileFormatException(System.Runtime.Serialization.SerializationInfo info, System.Runtime.Serialization.StreamingContext context) : base(info, context) { }
    }
}
