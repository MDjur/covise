#ifndef VRB_REMOTE_LAUCHER_SPAWN_PROGRAMM_H
#define VRB_REMOTE_LAUCHER_SPAWN_PROGRAMM_H

#include "coExport.h"

#include <vector>
#include <string>
namespace covise
{
    //execPath: executable path, args: first arg must be executable name, last arg must be nullptr;
    UTILEXPORT void spawnProgram(const char* execPath, const std::vector<const char *> &args, const std::vector<const char *> &env = {});

    //execPath: executable path, args: command line args
    UTILEXPORT void spawnProgram(const std::string & execPath, const std::vector<std::string> &args, const std::vector<std::string> &env = {});

    //execPath: executable path, debugCommands: coCoviseConfig::getEntry("System.CRB.DebugCommand"), args: first arg must be executable name, last arg must be nullptr;
    UTILEXPORT void spawnProgramWithDebugger(const std::string& execPath, const std::string &debugCommands,const std::vector<std::string>& args);
    //execPath: executable path, debugCommands: coCoviseConfig::getEntry("System.CRB.MemcheckCommand"), args: first arg must be executable name, last arg must be nullptr;
    UTILEXPORT void spawnProgramWithMemCheck(const std::string&, const std::string &debugCommands,const std::vector<std::string>& args);

    //returns the " " separated tokens from the commandLine string as a vector
    UTILEXPORT std::vector<std::string> parseCmdArgString(const std::string &commandLine);
    UTILEXPORT std::vector<const char*> cmdArgsToCharVec(const std::vector<std::string>& args);


} //namespace covise

#endif