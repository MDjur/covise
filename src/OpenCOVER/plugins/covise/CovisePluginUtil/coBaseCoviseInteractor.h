/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef CO_BASE_COVISE_INTERACTOR_H
#define CO_BASE_COVISE_INTERACTOR_H

/*! \file
 \brief  feedback parameter changes to COVISE

 \author
 \author (C)
         Computer Centre University of Stuttgart,
         Allmandring 30,
         D-70550 Stuttgart,
         Germany

 \date
 */

#include <util/coExport.h>

#include <cstring>
#include <cstdio>

#include <cover/coInteractor.h>

namespace opencover
{
class RenderObject;

//!< interact with the parameters of a single module
class COVISEPLUGINEXPORT coBaseCoviseInteractor : public coInteractor
{
public:
    //! record holding the state of a single parameter
    struct paraRec
    {
        const char *name;
        const char *type;
        const char *value;
    };

protected:
    char *d_feedbackInfo; //!< the part of the attrib created by ApplLib
    int d_refCount; //!< referenc count for managing lifetime
    char *d_objName; //!< the name of the DO with the attirbs
    RenderObject *d_object; //!< the DO with attrib
    char *d_moduleName; //!< the name of the module
    int d_moduleInstance; //!< the instance of the module
    char *d_moduleHost; //!< the hostname of the module
    char *d_pluginName; //!< the name of the Plugin to load for this Interactor
    char *d_infoString; //!< whatever the user put in the attribute

    /// this part is only there if coFeedback created the attribute

    unsigned int d_numPara; //!< number of parameters
    unsigned int d_numUser; //!< number of user strings
    paraRec *d_param; //!< Parameters
    char **d_user; //!< User strings
    char *d_splitString; //!< save splitted string in here

    // un-mask masked special chars ( \x becomes x )
    static void unMask(char *string);

    virtual void sendFeedback(const char *info, const char *key, const char *data = NULL) = 0;

public:
    coBaseCoviseInteractor(const char *n, RenderObject *o, const char *attrib);
    ~coBaseCoviseInteractor();

    /// returns true, if Interactor comes from same Module as intteractor i;
    bool isSameModule(coInteractor *i) const override
    {
        const coBaseCoviseInteractor *ci = dynamic_cast<coBaseCoviseInteractor *>(i);
        return ci && (strcmp(d_feedbackInfo, ci->d_feedbackInfo) == 0);
    }

    /// returns true, if Interactor is exactly the same as interactor i;
    bool isSame(coInteractor *i) const override
    {
        const coBaseCoviseInteractor *ci = dynamic_cast<coBaseCoviseInteractor *>(i);
        return ci && ((strcmp(d_feedbackInfo, ci->d_feedbackInfo) == 0) && (strcmp(d_moduleName, ci->d_moduleName) == 0));
    }

    /// execute the Module
    void executeModule() override;

    /// copy the Module to same host
    void copyModule() override;

    /// copy the Module to same host and execute the copied one
    void copyModuleExec() override;

    /// delete the Module
    void deleteModule() override;

    // --- all getParameter Functions
    //       - return -1 on fail (illegal type requested), 0 if ok
    //       - only work for coFeedback created parameter messages
    //       - do not change the value fields if type incorrect

    int getBooleanParam(unsigned int paraNo, int &value) const;
    int getIntScalarParam(unsigned int paraNo, int &value) const;
    int getFloatScalarParam(unsigned int paraNo, float &value) const;
    int getIntSliderParam(unsigned int paraNo, int &min, int &max, int &val) const;
    int getFloatSliderParam(unsigned int paraNo, float &min, float &max, float &val) const;
    int getIntVectorParam(unsigned int paraNo, int &numElem, int *&val) const;
    int getFloatVectorParam(unsigned int paraNo, int &numElem, float *&val) const;
    int getStringParam(unsigned int paraNo, const char *&val) const;
    int getChoiceParam(unsigned int paraNo, int &num, char **&labels, int &active) const;
    int getFileBrowserParam(unsigned int paraNo, char *&val) const;

    int getBooleanParam(const std::string &paraName, int &value) const override;
    int getIntScalarParam(const std::string &paraName, int &value) const override;
    int getFloatScalarParam(const std::string &paraName, float &value) const override;
    int getIntSliderParam(const std::string &paraName, int &min, int &max, int &val) const override;
    int getFloatSliderParam(const std::string &paraName, float &min, float &max, float &val) const override;
    int getIntVectorParam(const std::string &paraName, int &numElem, int *&val) const override;
    int getFloatVectorParam(const std::string &paraName, int &numElem, float *&val) const override;
    int getStringParam(const std::string &paraName, const char *&val) const override;
    int getChoiceParam(const std::string &paraName, int &num, char **&labels, int &active) const override;
    int getFileBrowserParam(const std::string &paraName, char *&val) const override;

    // --- set-Functions:

    /// set Boolean Parameter
    void setBooleanParam(const char *name, int val) override;

    /// set float scalar parameter
    void setScalarParam(const char *name, float val) override;

    /// set int scalar parameter
    void setScalarParam(const char *name, int val) override;

    /// set float slider parameter
    void setSliderParam(const char *name, float min, float max, float value) override;

    /// set int slider parameter
    void setSliderParam(const char *name, int min, int max, int value) override;

    /// set float Vector Param
    void setVectorParam(const char *name, int numElem, float *field) override;
    void setVectorParam(const char *name, float u, float v, float w) override;

    /// set int vector parameter
    void setVectorParam(const char *name, int numElem, int *field) override;
    void setVectorParam(const char *name, int u, int v, int w) override;

    /// set string parameter
    void setStringParam(const char *name, const char *val) override;

    /// set choice parameter, pos starts with 0
    void setChoiceParam(const char *name, int pos) override;
    void setChoiceParam(const char *name, int num, const char *const *list, int pos) override;

    /// set browser parameter
    void setFileBrowserParam(const char *name, const char *val) override;

    // name of the covise data object which has feedback attributes
    const char *getObjName() override { return d_objName; }

    // the covise data object which has feedback attributes
    RenderObject *getObject() override { return d_object; }

    // get the name of the module which created the data object
    const char *getPluginName() override { return d_pluginName; }

    // get the name of the module which created the data object
    const char *getModuleName() override { return d_moduleName; }

    // get the instance number of the module which created the data object
    int getModuleInstance() override { return d_moduleInstance; }

    // get the hostname of the module which created the data object
    const char *getModuleHost() override { return d_moduleHost; }

    // get the string the module sends
    const char *getInfo()
    {
        return d_infoString;
    }

    // -- The following functions only works for coFeedback attributes

    /// Get the number of Parameters
    int getNumParam() const override { return d_numPara; }

    /// Get the number of User Strings
    int getNumUser() const override { return d_numUser; }

    // get a parameter record: use interact->para(1)->name et al.
    const paraRec *getPara(unsigned int i) const;

    // get a parameter name
    const char *getParaName(unsigned int i) const;

    // get a parameter type
    const char *getParaType(unsigned int i) const;

    // get a parameter value string (better use getXxxParam() calls)
    const char *getParaValue(unsigned int i) const;

    // get a User-supplied string
    const char *getString(unsigned int i) const override;

    // dump this interactor's contents
    void print(FILE *outfile);
};
}
#endif
