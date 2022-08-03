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
    //This class contains a driver for the purposes of testing the file processing functionalities
    class MainClass
    {
        public static int Main()
        {
            DirectoryInfo testDirectory = new DirectoryInfo(@"C:\testInputs\");
            using (StreamWriter outputFile = new StreamWriter(@"C:\tmp\output.txt"))
            {
                outputFile.WriteLine("... Beginning Testing ...");
                outputFile.WriteLine();
                outputFile.WriteLine();

                foreach (FileInfo testFile in testDirectory.GetFiles())
                {
                    //test setup
                    outputFile.WriteLine("Current Test: " + testFile.Name);
                    OpenFOAMFileProcessor currTest;

                    //initial file processing
                    try
                    {
                        currTest = new OpenFOAMFileProcessor(testFile.FullName);
                        outputFile.WriteLine("File processing completed.");
                    }
                    catch (OpenFOAMFileFormatException e)
                    {
                        outputFile.WriteLine("File processing unsuccessful.  " + e.Message);
                        outputFile.WriteLine(); //spacing from the end will otherwise be skipped due to the continue
                        outputFile.WriteLine();
                        continue; //skip file information access testing since file was not processed successfully
                    }

                    //file name access
                    try { outputFile.WriteLine("File Name: " + currTest.getFilename()); }
                    catch (OpenFOAMFileFormatException e) { outputFile.WriteLine("File name access failed.  " + e.Message); }

                    //file version access
                    try { outputFile.WriteLine("File Version: " + currTest.getFileVersion()); }
                    catch (OpenFOAMFileFormatException e) { outputFile.WriteLine("File version access failed.  " + e.Message); }

                    //file format access
                    try { outputFile.WriteLine("File Format: " + currTest.getFileFormat()); }
                    catch (OpenFOAMFileFormatException e) { outputFile.WriteLine("File format access failed.  " + e.Message); }

                    //file class access
                    try { outputFile.WriteLine("File Class: " + currTest.getFileClass()); }
                    catch (OpenFOAMFileFormatException e) { outputFile.WriteLine("File class access failed.  " + e.Message); }

                    //file location access
                    try { outputFile.WriteLine("File Location: " + currTest.getFileLocation()); }
                    catch (OpenFOAMFileFormatException e) { outputFile.WriteLine("File location access failed.  " + e.Message); }

                    //file object access
                    try { outputFile.WriteLine("File Object: " + currTest.getFileObject()); }
                    catch (OpenFOAMFileFormatException e) { outputFile.WriteLine("File object access failed.  " + e.Message); }

                    //file contents access
                    Dictionary<string, object> fileContents = currTest.getFileContents();
                    outputFile.WriteLine("File Contents: ");
                    outputFile.Write(generateDictionaryPrintStatement(in fileContents, 1, false));

                    //add space after a completed test
                    outputFile.WriteLine();
                    outputFile.WriteLine();
                }

                outputFile.WriteLine("... Testing Completed ...");
                outputFile.WriteLine();
                outputFile.WriteLine();
            }

            return 0;
        }

        private static string generateDictionaryPrintStatement(in Dictionary<string, object> dict, int level, bool inList)
        {
            string printText = String.Empty;
            foreach (KeyValuePair<string, object> outerEntry in dict)
            {
                string line = String.Empty;
                if (!inList)
                    for (int spaces = 0; spaces < level * 2; spaces++)
                        line += " ";
                if (outerEntry.Value is Dictionary<string, object>)
                {
                    if (!inList)
                        printText += line + outerEntry.Key + ":\n";
                    else
                        printText += outerEntry.Key;
                    printText += generateDictionaryPrintStatement((Dictionary<string, object>)outerEntry.Value, level + 1, inList);
                } 
                else if (outerEntry.Value is List<object>)
                {
                    printText += line + outerEntry.Key + ": ";
                    List<object> list = (List<object>)outerEntry.Value;
                    if (!inList)
                        printText += generateListPrintStatement(ref list, level, true) + "\n";
                    else
                        printText += generateListPrintStatement(ref list, level, true);
                }
                else if (outerEntry.Value is string)
                    if (!inList)
                        printText += line + outerEntry.Key + ": " + outerEntry.Value + "\n";
                    else
                        printText += outerEntry.Key + ": " + outerEntry.Value;
                else
                    throw new OpenFOAMFileFormatException("Incorrect syntax in saved dictionary for key " + outerEntry.Key + ".");

            }
            return printText;
        }

        private static string generateListPrintStatement(ref List<object> list, int level, bool inList)
        {
            String printLine = "(";
            foreach (object item in list)
            {
                if (item is string)
                    printLine += item + ", ";
                else if (item is List<object>)
                {
                    List<object> newList = (List<object>)item;
                    printLine += generateListPrintStatement(ref newList, level, true) + ", ";
                }
                else if (item is Dictionary<string, object>)
                {
                    Dictionary<string, object> dict = (Dictionary<string, object>)item;
                    printLine += "{" + generateDictionaryPrintStatement(dict, level, true);
                    if (printLine.EndsWith("\n"))
                        printLine = printLine.Substring(0, printLine.Length - 1);
                    printLine += "}, ";
                }
                else
                    throw new OpenFOAMFileFormatException("Incorrect syntax in saved list for item " + item + ".");
            }
            if (printLine.EndsWith(", "))
                printLine = printLine.Substring(0, printLine.Length - 2);
            printLine += ")";
            return printLine;
        }
    }


    /// <summary>
    /// This class is designed to process config files written in the OpenFOAM syntax style and store the filename, file text (currently inaccessible outside of the class),
    /// file background data, and file contents (the last two of which are acheived through file processing in the constructor) in order to use the information from the file 
    /// to set base and default values.
    /// </summary>
    public class OpenFOAMFileProcessor
    {
        private string filename; //name of the file being processed by the instance of the class
        private Dictionary<string, object> fileData; //background information about the file itself
        private Dictionary<string, object> fileContents; //dictionary information stored in file

        /// <summary>
        /// Constructor that creates a new file processor instance for the specific file provided (by filename) and parses the file to populate all class fields
        /// </summary>
        /// <param name="filename">String containing the name of the file to be processed</param>
        public OpenFOAMFileProcessor(in string filename)
        {
            this.filename = filename;
            this.fileData = new Dictionary<string, object>();
            this.fileContents = new Dictionary<string, object>();

            string[] lines;
            try //read file (with automatic file opening and closing as part of utilized method)
            {
                lines = File.ReadAllLines(filename);
            }
            catch (Exception e) //error reading file
            {
                OpenFOAMDialogManager.ShowDialogException(e);
                return;
            }

            isolateFileData(ref lines);
            extractFileData(ref lines);
        }

        /// <summary>
        /// This is a private helper method for the constructor which performs preprocessing on the lines in the file to remove comments and leading and trailing whitespace.
        /// </summary>
        private void isolateFileData(ref string[] lines)
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
        private void extractFileData(ref string[] lines)
        {
            int lineNum = 0;
            while (lineNum < lines.Length)
            {
                if (String.IsNullOrWhiteSpace(lines[lineNum])) //skip any blank lines
                    lineNum++;
                else if (lineNum >= lines.Length) //this is a syntax error
                    throw new OpenFOAMFileFormatException("Improper or incomplete data contained in config file " + filename + " beginning at line " + ++lineNum + "."); //0 indexed array vs 1 indexed line numbers in file
                else if (lines[lineNum].ToLower().Equals("foamfile")) //this section contains the file data (lowercase to ensure correct text identification)
                {
                    lineNum = processFileData(lineNum, ref lines); //actually handles file data processing
                    fileContents.Add(getFileObject(), new Dictionary<string, object>()); //sets up single master dictionary in order to process singletons
                }
                else //this section contains the file contents 
                {
                    Dictionary<string, object> mainDictionary = (Dictionary<string, object>) fileContents[getFileObject()];
                    lineNum = processDictionaryEntry(lineNum, ref lines, ref mainDictionary);
                }
            }
        }

        /// <summary>
        /// This is a private helper method for the extractFileData method, which takes in line number expected to contain information about the file and processes it accordingly.
        /// </summary>
        /// <param name="lineNum">int indicating the current line that must be processed (in connection with any later lines related to it)</param>
        /// <returns>int indicating the new line number after processing all relevant information for the original line number</returns>
        /// <throws>OpenFOAMFileFormatException indicating that there is a syntax error in the file being processed if the information is not presented as expected</throws>
        private int processFileData(int lineNum, ref string[] lines)
        {
            lineNum++; //skip file data header
            while (lineNum < lines.Length && String.IsNullOrWhiteSpace(lines[lineNum]))
                lineNum++; //skip any blank lines
            if (lineNum >= lines.Length || !lines[lineNum].Equals("{"))
                throw new OpenFOAMFileFormatException("Improper file information syntax in config file " + filename + ".");
            else 
            {
                lineNum++; //skip opening brace
                while (lineNum < lines.Length && !lines[lineNum].Equals("}"))
                {
                    if (String.IsNullOrWhiteSpace(lines[lineNum]))
                    {
                        lineNum++;
                        continue; //skip any blank lines
                    }
                    string[] currLine = lines[lineNum].Split('\t');
                    string currKey = String.Empty;
                    string currVal = String.Empty;
                    bool missingSemicolon = true;
                    foreach (string segment in currLine)
                    {
                        String section = segment.Trim(); //remove remaining whitespace on segment
                        if (String.IsNullOrWhiteSpace(section))
                            continue; //skip any blank sections which only contained whitespace characters
                        else if (String.IsNullOrWhiteSpace(currKey))
                            currKey = section; //first section of text in the line is the key
                        else if (String.IsNullOrWhiteSpace(currVal))
                            if (section.EndsWith(";")) //parsing information with attached semicolon
                            {
                                currVal = section.Substring(0, section.Length - 1); //second section of text in the line is the value
                                missingSemicolon = false;
                            }
                            else //parsing information without a semicolon
                                currVal = section;
                        else if (missingSemicolon && section.Equals(";"))
                            missingSemicolon = false;
                        else //additional text in the line indicates a syntax error
                            throw new OpenFOAMFileFormatException("Improper file information syntax in config file " + filename + " at line number " + ++lineNum + "."); //0 indexed array vs 1 indexed line numbers in file
                    }
                    if (String.IsNullOrWhiteSpace(currKey) || String.IsNullOrWhiteSpace(currVal) || missingSemicolon)
                        throw new OpenFOAMFileFormatException("Improper file information syntax in config file " + filename + " at line number " + ++lineNum + "."); //0 indexed array vs 1 indexed line numbers in file
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
        private int processDictionaryEntry(int lineNum, ref string[] lines, ref Dictionary<string, object> parentDict)
        {
            lines[lineNum] = lines[lineNum].Trim();
            string[] currLine = lines[lineNum].Split('\t');
            if (currLine.Length == 1) //dictionary header identified
            {
                string key = lines[lineNum++];
                while (lineNum < lines.Length && String.IsNullOrWhiteSpace(lines[lineNum]))
                    lineNum++; //skip any blank lines
                if (lineNum >= lines.Length) //improper syntax identified
                    throw new OpenFOAMFileFormatException("Improper file information syntax in config file " + filename + ".");
                else if (lines[lineNum].Equals("("))
                    lineNum = processList(lineNum, ref lines, ref parentDict, key);
                else if (lines[lineNum].Equals("{")) //dictionary is identified
                    lineNum = processDictionary(lineNum, ref lines, ref parentDict, key);
                else
                    throw new OpenFOAMFileFormatException("Improper file information syntax in config file " + filename + ".");
            } 
            else //singleton entry identified
                lineNum = processDataEntry(lineNum, ref lines, ref parentDict);
            return lineNum;
        }

        private int processDataEntry(int lineNum, ref string[] lines, ref Dictionary<string, object> dict)
        {
            //check for blank lines
            if (String.IsNullOrWhiteSpace(lines[lineNum]))
                return ++lineNum; //skip any blank lines

            //split line
            string[] currLine = lines[lineNum].Split('\t');

            //check for data value entry
            string currKey = String.Empty;
            object currVal = String.Empty;
            bool missingSemicolon = true;
            foreach (string segment in currLine)
            {
                String section = segment.Trim(); //remove remaining whitespace on segment
                if (String.IsNullOrWhiteSpace(section))
                    continue; //skip any blank sections which only contained whitespace characters
                else if (String.IsNullOrWhiteSpace(currKey))
                    currKey = section; //first section of text in the line is the key
                else if (String.IsNullOrWhiteSpace((string)currVal))
                    if (section.EndsWith(";")) //parsing information with attached semicolon
                    {
                        currVal = section.Substring(0, section.Length - 1); //second section of text in the line is the value
                        if (((string)currVal).StartsWith("(")) //if currVal should be a list
                        {
                            currVal = processSingleLineList((string)currVal, false);
                        }
                        missingSemicolon = false;
                    }
                    else //parsing information without a semicolon
                        currVal = section;
                else if (missingSemicolon && section.Equals(";"))
                    missingSemicolon = false;
                else //additional text in the line indicates a syntax error
                    throw new OpenFOAMFileFormatException("Improper dictionary entry syntax in config file " + filename + " at line number " + ++lineNum + "."); //0 indexed array vs 1 indexed line numbers in file
            }
            if (currVal is string && String.IsNullOrWhiteSpace((string)currVal))
                lineNum = processDictionaryEntry(lineNum, ref lines, ref dict); //new nested dictionary
            else if (String.IsNullOrWhiteSpace(currKey) || missingSemicolon)
                throw new OpenFOAMFileFormatException("Improper dictionary entry syntax for data point in config file " + filename + " at line number " + ++lineNum + "."); //0 indexed array vs 1 indexed line numbers in file
            else
                dict.Add(currKey, currVal);
            return ++lineNum; //current line has been processed, so advance to the next line before continuing
        }

        private List<object> processSingleLineList(string listText, bool closed)
        {
            string[] listContent = listText.Substring(1, listText.Length - 1).Split(); //remove open and close parens
            List<object> list = new List<object>();
            int idx = 0;
            while (idx < listContent.Length && !listContent[idx].EndsWith(")"))
            {
                if (listContent[idx].StartsWith("("))
                    list.Add(processSingleLineList(listContent[idx].Substring(1), false));
                else if (listContent[idx].StartsWith("{"))
                    throw new OpenFOAMFileFormatException("Dictionaries cannot be declared within lists with the notation <key  (list);>, but this type of format is present in config file " + filename + ".");
                else
                    list.Add(listContent[idx++]);
            }
            if (listContent[idx].EndsWith(")"))
            {
                if (listContent[idx].Length > 1)
                    list.Add(listContent[idx].Substring(0, listContent[idx].Length - 1));
                if (closed)
                    throw new OpenFOAMFileFormatException("There are too many list closings without corresponding openings in cofig file " + filename + ".");
                else
                    closed = true;
            }
            if (closed)
                return list;
            else
                throw new OpenFOAMFileFormatException("There are insufficient list closings for the number of openings in config file " + filename + ".");
        }

        private int processDictionary(int lineNum, ref string[] lines, ref Dictionary<string, object> parentDict, string key)
        {
            lineNum++; //skip opening brace
            Dictionary<string, object> dict = new Dictionary<string, object>();
            while (lineNum < lines.Length && !lines[lineNum].Equals("}"))
                lineNum = processDataEntry(lineNum, ref lines, ref dict);
            if (lineNum >= lines.Length)
                throw new OpenFOAMFileFormatException("Improper dictionary syntax in config file " + filename + ".");
            else if (lines[lineNum].Equals("}"))
                lineNum++; //skip closing brace
            parentDict.Add(key, dict);
            return lineNum;
        }

        private int processDictionary(int lineNum, ref string[] lines, ref List<object> parentList)
        {
            lineNum++; //skip opening brace
            Dictionary<string, object> dict = new Dictionary<string, object>();
            while (lineNum < lines.Length && !lines[lineNum].Equals("}"))
                lineNum = processDataEntry(lineNum, ref lines, ref dict);
            if (lineNum >= lines.Length)
                throw new OpenFOAMFileFormatException("Improper dictionary syntax in config file " + filename + ".");
            else if (lines[lineNum].Equals("}"))
                lineNum++; //skip closing brace
            parentList.Add(dict);
            return lineNum;
        }

        private int processList(int lineNum, ref string[] lines, ref Dictionary<string, object> parentDict, string key)
        { 
            lineNum++; //skip opening brace
            List<object> list = new List<object>();
            while (lineNum < lines.Length && !lines[lineNum].StartsWith(")"))
                lineNum = processListEntry(lineNum, ref lines, ref list);
            if (lineNum >= lines.Length)
                throw new OpenFOAMFileFormatException("Improper list syntax in config file " + filename + ".");
            else if (!lines[lineNum].EndsWith(";"))
                throw new OpenFOAMFileFormatException("Improper list syntax in config file " + filename + " due to missing ending semicolon.");
            else
                lineNum++; //skip closing brace
            parentDict.Add(key, list);
            return lineNum;
        }

        private int processList(int lineNum, ref string[] lines, ref List<object> parentList)
        {
            lineNum++; //skip opening brace
            List<object> list = new List<object>();
            while (lineNum < lines.Length && !lines[lineNum].Equals(")") && !lines[lineNum].Equals(");"))
                lineNum = processListEntry(lineNum, ref lines, ref list);
            if (lineNum >= lines.Length)
                throw new OpenFOAMFileFormatException("Improper list syntax in config file " + filename + ".");
            else if (lines[lineNum].Equals(")"))
                lineNum++; //skip closing brace
            parentList.Add(list);
            return lineNum;
        }

        private int processListEntry(int lineNum, ref string[] lines, ref List<object> list)
        {
            //check for blank lines
            if (String.IsNullOrWhiteSpace(lines[lineNum]))
                return ++lineNum; //skip any blank lines

            //split line
            string[] currLine = lines[lineNum].Split('\t');

            //check for list value entry (or entries)
            foreach (string segment in currLine)
            {
                String section = segment.Trim(); //remove remaining whitespace on segment
                if (String.IsNullOrWhiteSpace(section))
                    continue; //skip any blank sections which only contained whitespace characters
                else if (section.Equals("(")) //multi-line list identified
                    lineNum = processList(lineNum, ref lines, ref list);
                else if (section.StartsWith("(")) //single-line list identified
                    list.Add(processSingleLineList(section, false));
                else if (section.Equals("{")) //dictionary identified
                    lineNum = processDictionary(lineNum, ref lines, ref list) - 1; //closing brace already skipped, so we must return to closing brace so it can be reskipped at the end of this method
                else
                    list.Add(section);
            }
            return ++lineNum; //current line has been processed, so advance to the next line before continuing
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
            if (fileData.ContainsKey("version") && fileData["version"] is string)
                return (string)fileData["version"];
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
            if (fileData.ContainsKey("format") && fileData["format"] is string)
                return (string)fileData["format"];
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
            if (fileData.ContainsKey("class") && fileData["class"] is string)
                return (string)fileData["class"];
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
            if (fileData.ContainsKey("location") && fileData["location"] is string)
                return (string)fileData["location"];
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
            if (fileData.ContainsKey("object") && fileData["object"] is string)
                return (string)fileData["object"];
            else
                throw new OpenFOAMFileFormatException("Object information not initialized properly in config file " + filename + ".");
        }

        /// <summary>
        /// This is a public getter method for the contents of the file (or the entirety of the private field fileContents).
        /// </summary>
        /// <returns>Dictionary<string, Dictionary<string, string>> containing the contents of the parsed file</returns>
        public Dictionary<string, object> getFileContents() => fileContents;
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
