/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef CO_TUI_SGBROWSER_TAB_H
#define CO_TUI_SGBROWSER_TAB_H

#include <util/coRestraint.h>
#include <util/coTypes.h>

#include <net/covise_connect.h>

#include "TUITab.h"
#include "TUILabel.h"
#include <QObject>
#include <QFrame>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <string>
#include <queue>
#include <map>
#include <vector>

#include <QCloseEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QMutex>
#include <QThread>
#include <QDir>
#include <QFileDialog>
#include <QFile>

#include "qtpropertyDialog.h"

#ifndef WIN32
#include <stdint.h>
#endif

class QTabWidget;

class QSignalMapper;
class Q3NetworkOperation;
class QSocketNotifier;

class nodeTree;
class nodeTreeItem;
class SGTextureThread;
class PropertyDialog;

namespace covise
{
class ServerConnection;
class Connection;
class ConnectionList;
class Message;
}

class TUISGBrowserTab : public QObject, public TUITab
{
    Q_OBJECT
public:
    TUISGBrowserTab(int id, int type, QWidget *w, int parent, QString name);
    virtual ~TUISGBrowserTab();
    virtual const char *getClassName() const;
    virtual void setValue(TabletValue type, covise::TokenBuffer &tb);

    void changeTexture(int, std::string);
    covise::Connection *getClient();;
    covise::Connection *getServer();;
    void send(covise::TokenBuffer &tb);
    bool isReceivingTextures()
    {
        return receivingTextures;
    };
    virtual void setColor(Qt::GlobalColor c)
    {
        TUITab::setColor(c);
    }

    // void handleAddNode(QTreeWidgetItem* item, QTreeWidgetItem* parent_item);
    // void handleRemoveNode(QTreeWidgetItem* item, QTreeWidgetItem* parent_item);
    // void handleMoveNode(QTreeWidgetItem* item, QTreeWidgetItem* oldParent_item, QTreeWidgetItem* newParent_item);

protected:
    void closeEvent(QCloseEvent *);

private:
    std::string lastNode;
    std::string tabeltsimPath;
    std::map<std::string, std::string> SimPath_SimName;
    //First CAD Item Second Sim Items
    std::map<std::string, std::vector<std::string> > CAD_SIM_Node;
    std::map<std::string, bool> CAD_Status;
    std::map<std::string, bool> SIM_Status;
    bool new_Sim(std::string, std::string);
    bool is_SimNode(std::string);
    bool decision(int, QString &, QColor &);
    void setColorState(QTreeWidgetItem *, bool, bool setColor = true);
    void CheckedItems(bool show);
    void CheckedItems(QString searchStr, bool show);
    void findItem();
    void sendSetShaderRequest(QTreeWidgetItem *item, const char *shaderName);
    void sendRemoveShaderRequest(QTreeWidgetItem *item);

    void sendAddNodeRequest(const char* parent_path, int node_type);
    void sendRemoveNodeRequest(const char* item_path, const char* parent_path);
    void sendMoveNodeRequest(const char* item_oldPath, const char* oldParent_path, const char* newParent_path, int dropIndex);
    void sendRenameNodeRequest(const char* item_path, const char* item_newName);

    void setCurrentNode(QTreeWidgetItem* focusItem);
    void setCurrentNode(QTreeWidgetItem* focusParentItem, int index);
    void setCurrentNode(QString focusPath_str);

    float ColorR, ColorG, ColorB;
    int polyMode;
    int selMode;
    bool root;
    bool showNodes;
    bool recursiveSel;
    bool find;
    bool firsttime;
    QFrame *frame;
    nodeTree *treeWidget;
    QLineEdit *findEdit;
    QPushButton *findButton;
    QPushButton *recursiveButton;
    QPushButton *showButton;
    QPushButton *hideButton;
    QPushButton *showhideButton;
    QPushButton *updateButton;
    QPushButton *colorButton;
    QCheckBox *showCBox;
    QCheckBox *selectCBox;
    QCheckBox *selModeCBox;
    QComboBox *selectionModeCB;
    QHBoxLayout *Treelayout;
    PropertyDialog *propertyDialog;
    QIcon wireON, wireOFF;
    covise::coRestraint *restraint;
    QAction *propAct;
    std::vector<QAction *> simu;
    std::map<std::string, QAction *> Sim_Act_Pair; //first Simualtionpath; second action button
    QAction *cad;
    QAction *load;
    int numItems;
    QString sourceData;
    QImage *image;
    QTimer *updateTimer;
    int firstTime;
    bool receivingTextures;
    bool currentTexture;
    int textureModes[21];
    int textureTexGenModes[21];
    int textureIndices[21];
    int receivedTextures;

    SGTextureThread *thread;
    QStringList buttonList;
    std::list<int> indexList;

    QString texturePluginDir;
    QString texturePluginTempDir;
    QString textureDir;
    QString currentDir;
    bool connectionClosed = false;

public slots:
    void updateScene();
    
    void updateItemState(QTreeWidgetItem *, int);
    void handleApply();
    void handleCloseDialog();
    void changeTexMode(int);

    void updateTextures();
    void sendChangeTextureRequest();
    void handleRemoveTex();
    void loadTexture();
    //void        finished();
    void updateTextureButtons();
    void handleClient(const covise::Message *msg);

    void handleGetShader();
    void handleSetShader();
    void handleRemoveShader();
    void handleStoreShader();
    void handleSetUniform();
    void handleSetVertex();
    void handleSetTessControl();
    void handleSetTessEval();
    void handleSetFragment();
    void handleSetGeometry();
    void handleNumVertChanged(int);
    void handleInputTypeChanged(int);
    void handleOutputTypeChanged(int);
    void handleShaderList(QListWidgetItem *);

    void handleAddNode(QTreeWidgetItem* parent_item, int node_type);
    void handleAddNodeGroup();
    void handleAddNodeMatrixTransform();
    // void handleAddNodeGeode();

    void handleRemoveCurrentNode();
    void handleRemoveNode(QTreeWidgetItem* item, QTreeWidgetItem* parent_item);
    void handleMoveNode(QTreeWidgetItem* item, QTreeWidgetItem* oldParent_item, QTreeWidgetItem* newParent_item, int dropIndex);
    void handleRenamedCurrentNode();
    void handleRenamingCurrentNode();

private slots:
    void itemChangedSlot(QTreeWidgetItem *, QTreeWidgetItem *);
    void centerObject();
    void loadFiles();
    void viewSim();
    void viewCad();
    void showhideDialog();
    void showAllNodes(bool);
    void setSelMode(bool);
    void selectAllNodes(bool);
    void setRecursiveSel(bool);
    void changeSelectionMode(int);
    void showCheckedItems();
    void hideCheckedItems();
    void showhideCheckedItems();
    void findItemSLOT();
    void updateSelection();
    void setColor();
    void updateExpand(QTreeWidgetItem *item);
    void updateItem();
    void itemProperties();
    void showhideSimItems(int state, const char *nodePath);
    void showNode(QTreeWidgetItem *item, bool visible);

    void checkChildrenCurrentNode();
    void uncheckChildrenCurrentNode();
};

class SGTextureThread : public QThread
{
public:
    SGTextureThread(TUISGBrowserTab *tab);
    void run();
    void enqueueGeode(int number, std::string geode);
    void setButtonNumber(int number);
    bool isSending();
    void terminateTextureThread();
    void lock();
    void unlock();

private:
    QMutex m_mutex;

    TUISGBrowserTab *tab;
    int buttonNumber;
    bool running;
    std::queue<int> buttonQueue;
    std::queue<std::string> geodeQueue;
};

class nodeTreeItem : public QTreeWidgetItem
{
public:
    nodeTreeItem(nodeTree *, const QString &, QString, QString, QColor, QString, int, QString, int);
    nodeTreeItem(nodeTreeItem *, const QString &, QString, QString, QColor, QString, int, QString, int);
    ~nodeTreeItem();
    void setData(int column, int role, const QVariant &value);
};

class nodeTree : public QTreeWidget
{
    Q_OBJECT

public:
    nodeTree(QWidget *parent = 0);
    ~nodeTree();

    void init();

    nodeTreeItem *findParent(QString);
    nodeTreeItem *search(QTreeWidgetItem *, QString);

    //mode = 0 findString else find selected
    QList<QTreeWidgetItem *> findString(QString searchStr = "", int mode = 0);
    QList<QTreeWidgetItem *> searchString(QTreeWidgetItem *item, QString searchStr = "", bool parentState = true, int mode = 0);

signals:
    void itemCheckStateChanged(QTreeWidgetItem *item, bool state);
    void itemDropped(QTreeWidgetItem* item, QTreeWidgetItem* oldParent_item, QTreeWidgetItem* newParent_item, int dropIndex);
    // void itemRenamed(QTreeWidgetItem* item);
    // void itemChanged(QTreeWidgetItem* item, int column);

protected:
    bool eventFilter(QObject* object, QEvent* event);
};
#endif
