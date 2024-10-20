
#include "Tool.h"
#include "ToolChanger/ToolChanger.h"
#include "VrmlNode.h"

#include <vrml97/vrml/VrmlNode.h>
#include <vrml97/vrml/VrmlNodeTransform.h>
#include <vrml97/vrml/VrmlNodeType.h>
#include <vrml97/vrml/VrmlNamespace.h>
#include <vrml97/vrml/VrmlSFVec3f.h>
#include <vrml97/vrml/VrmlSFFloat.h>
#include <vrml97/vrml/VrmlMFString.h>
#include <vrml97/vrml/VrmlMFFloat.h>
#include <vrml97/vrml/VrmlMFVec3f.h>
#include <vrml97/vrml/VrmlSFInt.h>
#include <vrml97/vrml/VrmlMFInt.h>
#include <vrml97/vrml/VrmlNodeChildTemplate.h>
#include <plugins/general/Vrml97/ViewerObject.h>
#include <OpcUaClient/opcua.h>
#include <cover/ui/Menu.h>

enum UpdateMode
{
    All,
    AllOncePerFrame,
    UpdatedOncePerFrame
};

class Machine 
{
public:
    Machine(MachineNodeBase *node);


    void move(int axis, float value);
    bool arrayMode() const;
    void update(UpdateMode updateMode);
    void setUi(opencover::ui::Menu *menu, opencover::config::File *file);
    void pause(bool state);
    osg::MatrixTransform *getToolHead() const;

private:
    bool m_rdy = false;
    MachineNodeBase *m_machineNode;
    opcua::Client *m_client;
    std::vector<opencover::opcua::ObserverHandle> m_valueIds;
    size_t m_index = 0;
    std::unique_ptr<SelfDeletingTool> m_tool;
    opencover::ui::Menu *m_menu;
    opencover::config::File *m_configFile;

    bool addTool();
    void connectOpcua();
    bool updateMachine(bool haveTool, UpdateMode updateMode);

};
