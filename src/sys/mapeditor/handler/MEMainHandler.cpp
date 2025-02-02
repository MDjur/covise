/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef _WIN32
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#endif

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLibrary>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPushButton>
#include <QScreen>
#include <QStyleFactory>
#include <QTimer>
#include <QUrl>
#include <QtGlobal>

#include <config/CoviseConfig.h>
#include <covise/covise_appproc.h>
#include <covise/covise_msg.h>
#include <messages/NEW_UI.h>
#include <net/covise_host.h>
#include <qtutil/NonBlockingDialogue.h>
#include <qtutil/Qt5_15_deprecated.h>
#include <util/covise_version.h>

#include "MEFavoriteListHandler.h"
#include "../covise/MEFileBrowser.h"
#include "MEFileBrowserListHandler.h"
#include "MEHostListHandler.h"
#include "MELinkListHandler.h"
#include "MEMainHandler.h"
#include "../covise/MEMessageHandler.h"
#include "MENodeListHandler.h"
#include "../covise/MERemotePartner.h"
#include "modulePanel/MEModulePanel.h"
#include "nodes/MECategory.h"
#include "nodes/MENode.h"
#include "ports/MEDataPort.h"
#include "ports/MEParameterPort.h"
#include "ports/METimer.h"
#include "ui_MEAboutDialog.h"
#include "widgets/MEDialogTools.h"
#include "widgets/MEFavorites.h"
#include "widgets/MEGraphicsView.h"

#include "widgets/MESessionSettings.h"
#include "widgets/MEUserInterface.h"

#include <chrono>
#include <thread>
#include <cassert>

QColor MEMainHandler::s_highlightColor;
QColor MEMainHandler::s_paramColor, MEMainHandler::s_reqMultiColor, MEMainHandler::s_reqDataColor;
QColor MEMainHandler::s_multiColor, MEMainHandler::s_dataColor, MEMainHandler::s_chanColor;
QColor MEMainHandler::s_requestedColor, MEMainHandler::s_optionalColor, MEMainHandler::s_defColor, MEMainHandler::s_dependentColor;

QFont MEMainHandler::s_normalFont, MEMainHandler::s_boldFont, MEMainHandler::s_itBoldFont, MEMainHandler::s_italicFont;
QPalette MEMainHandler::defaultPalette;
QSize MEMainHandler::screenSize;

QString MEMainHandler::framework;

static QString expandTilde(QString path)
{
    if (!path.startsWith("~"))
        return path;

#ifdef _WIN32
    QString home;
    wchar_t *wstr = _wgetenv(L"USERPROFILE");
    wchar_t wbuf[MAX_PATH];
    if (wstr)
    {
        DWORD retval = GetShortPathNameW(wstr, wbuf, MAX_PATH);
        if (retval != 0)
        {
            home = QString::fromWCharArray(wbuf);
        }
        else
        {
            // fallback for the very unlikely case that GetShortPathNameW() fails
            home = "C:\\";
        }
    }
    else
    {
        // try TEMP
        wstr = _wgetenv(L"TEMP");
        if (wstr)
        {
            DWORD retval = GetShortPathNameW(wstr, wbuf, MAX_PATH);
            if (retval != 0)
            {
                home = QString::fromWCharArray(wbuf);
            }
            else
            {
                // fallback for the very unlikely case that GetShortPathNameW() fails
                home = "C:\\";
            }
        }
        else
        {
            home = "C:\\";
        }
    }
#else
    QString home = QString(getenv("HOME"));
#endif
    return home + path.mid(1);
}



MEMainHandler *MEMainHandler::singleton = nullptr;

/*!
    \class MEMainHandler
    \brief Handler for MEUserInterface, distribute message received by MEMessageHandler
*/

MEMainHandler::MEMainHandler(int argc, char *argv[], std::function<void(void)> quitFunc)
    : QObject()
    , m_cfg_QtStyle("System.UserInterface.QtStyle")
    , m_quitFunc(quitFunc)
    , m_mapEditorConfig("mapeditor")
    , m_guiSettings("HLRS", "MapEditor")
    , cfg_DeveloperMode(m_mapEditorConfig.value("General", "DeveloperMode", false))
    , cfg_storeWindowConfig(m_mapEditorConfig.value("General", "StoreLayout", false))
    , cfg_ErrorHandling(m_mapEditorConfig.value("General", "ErrorOutput", false))
    , cfg_HideUnusedModules(m_mapEditorConfig.value("General", "HideUnusedModules", true))
    , cfg_AutoConnect(m_mapEditorConfig.value("General", "AutoConnectHosts", true))
    , cfg_TopLevelBrowser(m_mapEditorConfig.value("General", "TopLevelBrowser", true))
    , cfg_TabletUITabs(m_mapEditorConfig.value("General", "TabletUITabs", true))
    , cfg_AutoSaveTime(m_mapEditorConfig.value("Saving", "AutoSave", int64_t(120)))
    , cfg_ModuleHistoryLength(m_mapEditorConfig.value("Saving", "ModuleHistoryLength", int64_t(50)))
    , cfg_NetworkHistoryLength(m_mapEditorConfig.value("Saving", "NetworkHistoryLength", int64_t(10)))
    , cfg_GridSize(m_mapEditorConfig.value("VisualProgramming", "SnapFactor", int64_t(1)))
    , cfg_HighColor(m_mapEditorConfig.value("VisualProgramming", "HighlightColor", std::string("red")))
    , cfg_HostColors(m_mapEditorConfig.array("VisualProgramming", "HostColors", std::vector<std::string>{"LightBlue", "PaleGreen", "khaki", "LightSalmon", "LightYellow", "LightPink"}))
    , m_messageHandler(argc, argv)
    , m_mapEditor(this)
{

    m_mapEditorConfig.setSaveOnExit(true);
    // init some variables
    singleton = this;
    localHost = "";
    m_mapName = "";

    screenSize = qApp->primaryScreen()->availableSize();

    framework = "COVISE";
    pm_logo = QPixmap(":/icons/covise.xpm");

    // define pixmaps
    pm_bullet = QPixmap(":/icons/bullet.png");
    pm_bulletmore = QPixmap(":/icons/bullet_go.png");
    pm_bookopen = QPixmap(":/icons/book_open32.png");
    pm_bookclosed = QPixmap(":/icons/book32.png");
    pm_folderclosed = QPixmap(":/icons/dirclosed-32.png");
    pm_folderopen = QPixmap(":/icons/diropen-32.png");
    pm_forward = QPixmap(":/icons/next32.png");
    pm_pinup = QPixmap(":/icons/pinup.xpm");
    pm_pindown = QPixmap(":/icons/pindown.xpm");
    pm_file = QPixmap(":/icons/file.png");
    pm_help = QPixmap(":/icons/help32.png");
    pm_stop = QPixmap(":/icons/stop.png");
    pm_copy = QPixmap(":/icons/copy.png");
    pm_exec = QPixmap(":/icons/execall.png");
    pm_exec2 = QPixmap(":/icons/execonchange.png");
    pm_table = QPixmap(":/icons/table.png");
    pm_collapse = QPixmap(":/icons/collapse.png");
    pm_expand = QPixmap(":/icons/expand.png");
    pm_lighton = QPixmap(":/icons/light_on.png");
    pm_lightoff = QPixmap(":/icons/light_off.png");
    pm_colorpicker = QPixmap(":/icons/colorpicker.png");
    pm_master = QPixmap(":/icons/master32.png");
    pm_adduser = QPixmap(":/icons/add_user.png");
    pm_addhost = QPixmap(":/icons/add_computer.png");

    // define colors for ports
    s_requestedColor.setRgb(255, 20, 150);
    s_defColor.setRgb(67, 119, 255);
    s_dependentColor.setRgb(255, 165, 0);
    s_optionalColor.setRgb(24, 139, 34);

    s_paramColor.setNamedColor("#FFFF00"); // yellow
    s_reqMultiColor.setNamedColor("#FF1493"); // DeepPink
    s_multiColor.setNamedColor("#9932CC"); // DarkOrchid
    s_reqDataColor.setNamedColor("#4169E1"); // RoyalBlue
    s_dataColor.setNamedColor("#32CD32"); // LimeGreen
    s_chanColor.setNamedColor("#F5DEB3"); // Wheat

    // set proper fonts
    s_normalFont.setWeight(QFont::Normal);
    s_italicFont.setWeight(QFont::Normal);
    s_italicFont.setItalic(true);
    s_boldFont.setWeight(QFont::Bold);
    s_itBoldFont.setWeight(QFont::Bold);
    s_itBoldFont.setItalic(true);

    // calculate max slider width
    QLabel *l = new QLabel("0.12345678912345");
    l->hide();
    m_sliderWidth = l->sizeHint().width();
    delete l;
    l = NULL;

    // initialize needed handler
    MENodeListHandler::instance();
    MEHostListHandler::instance();
    MELinkListHandler::instance();
    MEFileBrowserListHandler::instance();

    // connect to controller
    // or start in m_standalone mode

    // get some infos out of the config file
    readConfigFile();

    // assemble list of autosaved files
    QString dotdirpath = QDir::homePath();
#ifdef _WIN32
    dotdirpath += "/" + framework.toUpper();
#else
    dotdirpath += "/." + framework.toLower();
#endif
    QDir dotdir(dotdirpath);
    std::cerr << "autosave path " << dotdirpath.toStdString() << std::endl;
    QStringList namefilter;
    namefilter << "autosave*.net";
    dotdir.setNameFilters(namefilter);
    QStringList autosavefiles = dotdir.entryList();
    QFileInfoList autosaveinfo = dotdir.entryInfoList();
    QMap<QDateTime, QAction *> autosaveMap;
    for (int i = 0; i < autosavefiles.count(); ++i)
    {
        QString file = autosavefiles[i].section("/", -1);
        QStringList l = file.split("-");
        if (l.count() == 3)
        {
            QString ip = l[1];
            QString pid = l[2].section(".", 0, 0);
            if (ip != covise::Host().getAddress()
#ifndef _WIN32
                || kill(pid.toInt(), 0) != 0
#endif
                )
            {
                QAction *ac = new QAction(autosaveinfo[i].lastModified().toString() + " (" + ip + ")", 0);
                autosaveNameMap[ac] = dotdirpath + "/" + autosavefiles[i];
                connect(ac, SIGNAL(triggered(bool)), this, SLOT(openNetworkFile(bool)));
                autosaveMap.insert(autosaveinfo[i].lastModified(), ac);
            }
        }
    }
    foreach (QAction *ac, autosaveMap.values())
        autosaveMapList.prepend(ac);

    m_deleteAutosaved_a = new QAction("Delete All Autosaved Maps", 0);
    connect(m_deleteAutosaved_a, SIGNAL(triggered(bool)), this, SLOT(deleteAutosaved(bool)));

    // create the user interface
    m_mapEditor.init();

    if (!autosaveMapList.empty())
    {
        if (autosaveMapList.count() == 1)
        {
            m_mapEditor.printMessage("There is one autosaved network map available from the File->Open Autosaved menu");
        }
        else
        {
            m_mapEditor.printMessage(QString("There are %1 autosaved network maps available from the File->Open Autosaved menu").arg(autosaveMapList.count()));
        }
    }

    // check if online help is available
    checkHelp();

    // connect to controller
    init();

    // start the timer for saving  the map after a given time
    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, SIGNAL(timeout()), this, SLOT(autoSaveNet()));
    m_autoSaveTimer->start(cfg_AutoSaveTime->value() * 1000);

    // get screen size and default palette
    defaultPalette = QApplication::palette();

    // set a proper window title
    m_mapEditor.setWindowTitle(generateTitle(""));
    m_mapEditor.setWindowFilePath("");
    MEModulePanel::instance()->setWindowTitle(generateTitle("Module Parameters"));
    if (m_helpViewer)
        m_helpViewer->setWindowTitle(generateTitle("Help Viewer"));
    MEGraphicsView::instance()->viewAll();
}

MEMainHandler *MEMainHandler::instance()
{
    assert(singleton);
    return singleton;
}


//!
//! set the local user and host, contact controller
//!
void MEMainHandler::init()
{

// get local user name
    localUser = covise::coCoviseConfig::getEntry("value", "COVER.Collaborative.UserName", covise::Host::getUsername()).c_str();

    if (!m_messageHandler.isStandalone())
    {
        localHost = QString::fromStdString(covise::Host::lookupHostname(m_messageHandler.getUIF()->get_hostname()));
        localHost = localHost.section('.', 0, 0);
        localIP = QString::fromStdString(covise::Host::lookupIpAddress(m_messageHandler.getUIF()->get_hostname()));

        // send dummy message to tell the controller that it is safe now to send messages
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        m_messageHandler.dataReceived(1);
#else
        m_messageHandler.dataReceived(QSocketDescriptor(1), QSocketNotifier::Type::Read);
#endif

        // tell crb if we are ready for an embedded ViNCE renderer
        if (cfg_ImbeddedRenderer && cfg_ImbeddedRenderer->value())
        {
            embeddedRenderer();
        }
        else
        {
            QString tmp = "RENDERER_IMBEDDED_POSSIBLE\n" + localIP + "\n" + localUser + "\nFALSE";
            m_messageHandler.sendMessage(covise::COVISE_MESSAGE_MSG_OK, tmp);
        }
    }
}

//!
void MEMainHandler::embeddedRenderer()
//!
{
    // get sgrender lib
    m_libFileName = getenv("COVISEDIR");
    m_libFileName.append("/");
    m_libFileName.append(getenv("ARCHSUFFIX"));

#ifdef _WIN32
    m_libFileName.append("/lib/sgrenderer");
#else
    m_libFileName.append("/lib/libsgrenderer");
#endif

    QLibrary *lib = new QLibrary(m_libFileName);
    lib->unload();
    if (!lib->isLoaded())
    {
        QString tmp = "RENDERER_IMBEDDED_POSSIBLE\n" + localIP + "\n" + localUser + "\nTRUE";
        m_messageHandler.sendMessage(covise::COVISE_MESSAGE_MSG_OK, tmp);
    }
    else
    {
        QString tmp = "RENDERER_IMBEDDED_POSSIBLE\n" + localIP + "\n" + localUser + "\nFALSE";
        qWarning() << "MEMainHandler::startRenderer err: lib not found";
        m_messageHandler.sendMessage(covise::COVISE_MESSAGE_MSG_OK, tmp);
    }
}

//!
//! init (start) a module,  module was initiated by a click (NOT drop)
//!
void MEMainHandler::initModule()
{
    // object that sent the signal
    MEFavorites *fav = (MEFavorites *)sender();

    QString text = fav->getStartText();

    // get normal info out of textstring
    QString hostname = text.section(':', 0, 0);
    QString username = text.section(':', 1, 1);
    QString category = text.section(':', 2, 2);
    QString modulename = text.section(':', 3, 3);

    // send signal which is caught from MEModuleTree
    text = category + ":" + modulename;
    emit usingNode(text);

    // send message to controller to start the module
    QSize size = MEGraphicsView::instance()->viewport()->size();
    QPointF pp = MEGraphicsView::instance()->mapToScene(size.width() / 2, size.height() / 2);

    requestNode(modulename, hostname, (int)pp.x(), (int)pp.y(), NULL, MEMainHandler::NORMAL);
}

//!
//! read config file config.xml, mapeditor.xml &  the history path
//!
void MEMainHandler::readConfigFile()
{

// default values

    if (!m_cfg_QtStyle.hasValidValue())
#ifdef _WIN32
        m_cfg_QtStyle = "Windows";
#else
        m_cfg_QtStyle = "";
#endif



    s_highlightColor.setNamedColor(cfg_HighColor->value().c_str());

    // set layout style
    if (m_cfg_QtStyle.hasValidValue() && !std::string(m_cfg_QtStyle).empty())
    {
        QStyle *s = QStyleFactory::create(cfg_QtStyle());
        if (s)
            QApplication::setStyle(s);
    }

// create the default path for storing files
    QString frame = "covise";

    cfg_SavePath = "~/." + frame;
#ifdef _WIN32
    cfg_SavePath = "~\\" + frame;
#endif

    // look if this save path exists
    // otherwise make it
    QDir d(expandTilde(cfg_SavePath));
    if (!d.exists())
    {
        d.mkdir(expandTilde(cfg_SavePath));
        std::cout << "MapEditor config-path: " << expandTilde(cfg_SavePath).toStdString().c_str() << " does not exist --> created !!" << std::endl;
    }

    auto networkHistoryV = getUserBehaviour().value("NetworkHistory");
    if(!networkHistoryV.isValid())
        return;
    for (const auto &filename : networkHistoryV.toStringList())
    {
        QAction *ac = new QAction(filename, 0);
        connect(ac, SIGNAL(triggered(bool)), this, SLOT(openNetworkFile(bool)));
        recentMapList.append(ac);
    }
}

//!
//! generate the title for subwindows
//!

QString MEMainHandler::generateTitle(const QString &text)
{
    QString title = framework + " Map Editor (" + localUser + "@" + localHost + ")";
    if (!text.isEmpty())
        title += " - " + text;
    title += "[*]";
    return title;
}

//!
//! insert a new module in the history list
//!-
void MEMainHandler::insertNetworkInHistory(const QString &insertText)
{

    QStringList networkHistory;
    auto networkHistoryV = getUserBehaviour().value("NetworkHistory");
    if(networkHistoryV.isValid())
        networkHistory = networkHistoryV.toStringList();
    // insert text

    if(networkHistory.contains(insertText))
        return;

    // insert current item
    networkHistory.push_front(insertText);

    // remove last item when reaching list maximum
    if (networkHistory.size() > cfg_NetworkHistoryLength->value())
        networkHistory.pop_back();

    // update list in mapeditor.xml
    getUserBehaviour().setValue("NetworkHistory", networkHistory);

    QAction *ac = new QAction(insertText, 0);
    connect(ac, SIGNAL(triggered(bool)), this, SLOT(openNetworkFile(bool)));
    recentMapList.prepend(ac);
    m_mapEditor.insertIntoOpenRecentMenu(ac);
}

void MEMainHandler::storeCategoryExpandedState()
{
    MEHost *nptr = MEHostListHandler::instance()->getFirstHost();
    QStringList expandedCategories;
    foreach (MECategory *cptr, nptr->catList)
    {
        QTreeWidgetItem *category = cptr->getCategoryItem();
        if (category->isExpanded())
            expandedCategories.push_back(cptr->getName());
    }
    getUserBehaviour().setValue("ExpandedCategories", expandedCategories);
    // store window configuration
    m_mapEditor.storeSessionParam(cfg_storeWindowConfig->value());
    m_mapEditorConfig.save();

}

//!
//! look if online help is available
//!

void MEMainHandler::checkHelp()
{

    QString name = framework + "_PATH";

    // try to get COVISEDIR
    QString helpdir = getenv(name.toLatin1().data());
    if (helpdir.isEmpty())
    {
        m_mapEditor.printMessage("Environmental variable " + name + " not set\n");
        return;
    }

#ifdef _WIN32
    QStringList list = helpdir.split(";");
#else
    QStringList list = helpdir.split(":");
#endif

    // try to get help from local file system
    // checking COVISE_PATH
    QString onlineDocPath = "/doc/html";
    for (unsigned int i = 0; i < (unsigned int)list.count(); i++)
    {
        onlineDir = list[i];
        onlineDir.append(onlineDocPath);

        // test the directory
        QString tmp = onlineDir + "/index.html";
        QFile file(tmp);
        if (file.exists())
        {
            m_helpFromWeb = false;
            m_helpViewer = std::make_unique<MEHelpViewer>();
            m_helpViewer->init();
            return;
        }
    }

    // get help from webserver
    onlineDir = "https://fs.hlrs.de/projects/covise/doc/html";
    m_mapEditor.printMessage("Online help not found in local document search path \"" + helpdir + "\", falling back to \"" + onlineDir + "\"");
    if (QUrl(onlineDir).isValid())
    {
        m_helpFromWeb = true;
        m_helpViewer = std::make_unique<MEHelpViewer>();
        m_helpViewer->init();
    }
    else
        m_mapEditor.printMessage("Online help also not found on webserver");
}

void MEMainHandler::reportbug()
{
    if (!QDesktopServices::openUrl(QUrl("https://github.com/hlrs-vis/covise/issues/new")))
    {
        m_mapEditor.printMessage("Failed to open web page \"https://github.com/hlrs-vis/covise/issues/new\"");
    }
}

void MEMainHandler::help() { onlineCB("/index.html"); }
void MEMainHandler::tutorial() { onlineCB("/tutorial/index.html"); }
void MEMainHandler::usersguide() { onlineCB("/usersguide/index.html"); }
void MEMainHandler::moduleguide() { onlineCB("/modules/index.html"); }
void MEMainHandler::progguide() { onlineCB("/programmingguide/index.html"); }
void MEMainHandler::mapeditor() { onlineCB("/usersguide/mapeditor/index.html"); }

void MEMainHandler::onlineCB(const QString &html)
{
    // try again
    if (!m_helpViewer)
        checkHelp();

    if (m_helpViewer && !m_helpFromWeb)
    {
        QDir d(onlineDir);
        if (d.exists())
        {
            m_helpViewer->newSource(onlineDir + html);
        }
    }

    else
    {
        if (QUrl(onlineDir + html).isValid())
        {
            m_helpViewer->newSource(onlineDir + html);
        }
    }
}

//!
//! get current map name
//!
QString MEMainHandler::getMapName()
{
    return m_mapName.section('/', -1, -1);
}

//!
//! get current map path
//!
QString MEMainHandler::getMapPath()
{
    return m_mapName.section('/', 0, -2);
}

//!
//! store current map name
//!
void MEMainHandler::storeMapName(const QString &name)
{
    m_mapName = name;
    m_mapEditor.setWindowFilePath(name);
    if (m_mapEditor.hasMiniGUI())
    {
        QString tmp = "Current file: " + name;
        m_mapEditor.showMapName(tmp);
    }
    else
    {
        m_mapEditor.setWindowTitle(generateTitle(name));
    }
}

//!
//! open a network file that a user has dropped to an area
//!
void MEMainHandler::openDroppedMap(const QString &text)
{
    if (text.endsWith(".net"))
        openNetworkFile(text);
}

//!
//! chat line callback
//!
void MEMainHandler::chatCB()
{
    QString text = m_mapEditor.getChatLine()->text();
    if (text.isEmpty())
        return;

    QStringList buffer;
    QString data;
    buffer << "CHAT" << localHost << text;
    data = buffer.join("\n");
    m_messageHandler.sendMessage(covise::COVISE_MESSAGE_INFO, data);
    buffer.clear();
    m_mapEditor.getChatLine()->clear();
}

//!
//! delete all autosaved files
//!
void MEMainHandler::deleteAutosaved(bool)
{
    m_mapEditor.clearOpenAutosaveMenu();
    QList<QString> files = autosaveNameMap.values();
    for (int i = 0; i < files.count(); ++i)
    {
        QFile file(files[i]);
        if (file.exists() && !file.remove())
        {
            m_mapEditor.printMessage("Deleting " + files[i] + " failed");
        }
    }
}

//!
//! open a network file given in Open Recent
//!
void MEMainHandler::openNetworkFile(bool)
{
    // get action, text contains the modulename
    QAction *ac = (QAction *)sender();
    QString filename = ac->text();

    if (autosaveNameMap.contains(ac))
        filename = autosaveNameMap[ac];

    openNetworkFile(filename);
}

void MEMainHandler::openNetworkFile(QString filename)
{
    // if canvas is not empty and not saved ask if the user will really do that
    if (m_loadedMapWasModified && !MENodeListHandler::instance()->isListEmpty())
    {
        if (QMessageBox::question(0, "Map Editor",
                                  "The current dataflow network will be replaced. Do you want to continue?",
                                   (QMessageBox::StandardButton::Open | QMessageBox::StandardButton::Cancel), QMessageBox::StandardButton::Open) == QMessageBox::StandardButton::Cancel)
            return;
    }

// open file
    QString tmp;
    tmp = "UPDATE_LOADED_MAPNAME\n" + filename;
    m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, tmp);
    tmp = "NEW\n";
    m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, tmp);
    tmp = "OPEN\n" + filename;
    m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, tmp);

    setMapModified(false);
}

//!
//! user wants an undo action
//!
void MEMainHandler::undoAction()
{
    m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, "UNDO");
}

//!
//! open an existing net
//!
void MEMainHandler::openNet()
{
    // if canvas is not empty and not saved ask if the user will really do that
    if (m_loadedMapWasModified && !MENodeListHandler::instance()->isListEmpty())
    {
        if (QMessageBox::question(0, "Map Editor",
                                  "The current module network will be replaced. Do you want to continue?",
                                  (QMessageBox::StandardButton::Open | QMessageBox::StandardButton::Cancel), QMessageBox::StandardButton::Open) == QMessageBox::StandardButton::Cancel)
            return;
    }

    openBrowser(framework + " - Remote File Browser (OPEN)", MEFileBrowser::OPENNET);
}

//!
//! save the current map
//!
void MEMainHandler::saveNet()
{
    if (!m_mapName.isEmpty())
    {
        saveNetwork(m_mapName);
        insertNetworkInHistory(m_mapName);
    }

    else
        openBrowser(framework + " - Remote File Browser (SAVE)", MEFileBrowser::SAVEASNET);

    setMapModified(false);
}

//!
//! save the current map
//!
void MEMainHandler::saveAsNet()
{
    openBrowser(framework + " - Remote File Browser (SAVE AS)", MEFileBrowser::SAVEASNET);
    setMapModified(false);
}

//!
//! create main filebrowser
//!
void MEMainHandler::openBrowser(const QString &title, int mode)
{
    m_mapEditor.openBrowser(title, m_mapName, mode);
}

//!
//! auto save the current map
//!
void MEMainHandler::autoSaveNet()
{
    if (!m_inMapLoading && m_autoSave)
    {
        m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, "AUTOSAVE");
        m_autoSave = false;
    }
}

//!
//! store the current module network
//!
void MEMainHandler::saveNetwork(const QString &filename)
{
    QStringList buffer;
    buffer << "SAVE" << filename;
    QString tmp = buffer.join("\n");
    m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, tmp);
    buffer.clear();
}

//!
//! current network file was changed by an user operation
//!
void MEMainHandler::mapWasChanged(const QString &text)
{
    Q_UNUSED(text);
    m_autoSave = true;
    setMapModified(true);
}

//!
//! execute the whole map
//!
void MEMainHandler::execNet()
{
    m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, "EXEC\n");
    MEMainHandler::instance()->execTriggered();
}

//!
//! add a new partner
//!
void MEMainHandler::addPartner()
{
    if (!m_addPartnerDialog)
    {
        m_addPartnerDialog = new MERemotePartner{&m_mapEditor};
        connect(m_addPartnerDialog, &MERemotePartner::clientAction, this, [this](const covise::ClientInfo &client)
                {
                    requestPartnerAction(client.style, std::vector<int>{client.id});
                });
    }
    {
        std::lock_guard<std::mutex> g{m_remotePartnerMutex};
        m_addPartnerDialog->setPartners(m_remotePartners);
        m_addPartnerDialog->show();
    }
}

void MEMainHandler::requestPartnerAction(covise::LaunchStyle launchStyle, const std::vector<int> &clients){

    int timeout = 0;
    covise::NEW_UI_HandlePartners msg{launchStyle, timeout, clients};
    covise::sendCoviseMessage(msg, *MEMessageHandler::instance());
}

void MEMainHandler::deleteSelectedNodes()
{
    m_mapEditor.deleteSelectedNodes();

    // look if all nodes are gone
    // reset mapEditor
    if (MENodeListHandler::instance()->isListEmpty())
        reset();
}

//!
//! execute on change callback
//!
void MEMainHandler::changeCB(bool state)
{
    if (state == m_executeOnChange)
        return;
    m_executeOnChange = state;
}

//!
//! execute on change callback
//!
void MEMainHandler::changeExecButton(bool state)
{
    m_executeOnChange = state;
    m_mapEditor.changeExecButton(state);
}


//!
//! close the application
//!
void MEMainHandler::closeApplication(QCloseEvent *ce)
{
    // slave UI wants to quit session
    if (!m_masterUI)
    {
        if (canQuitSession())
        {
            auto host = MEHostListHandler::instance()->getHost(localIP, localUser);
            if (host)
            {
                covise::NEW_UI_HandlePartners pMsg{covise::LaunchStyle::Disconnect, 0, std::vector<int>{host->clientId()}};
                covise::sendCoviseMessage(pMsg, m_messageHandler);
            }
        }

        ce->ignore();
        return;
    }

    m_waitForClose = false;

    // ask the user what to do with the modified network map
    if (m_loadedMapWasModified)
    {
        QMessageBox msgBox(QMessageBox::Question, framework + " Map Editor",  "Save your changes before closing the " + framework + " session?");
        auto saveAndQuit = msgBox.addButton("Save && Quit", QMessageBox::ActionRole);
        auto quit = msgBox.addButton("Quit", QMessageBox::AcceptRole);
        auto cancel = msgBox.addButton("Cancel", QMessageBox::RejectRole);
        msgBox.exec();
        if (msgBox.clickedButton() == cancel)
        {
            ce->ignore();
        } else if (msgBox.clickedButton() == saveAndQuit)
        {

            storeCategoryExpandedState();

            // overwrite existing network
            if (!m_mapName.isEmpty())
            {
                saveNetwork(m_mapName);
                m_messageHandler.sendMessage(covise::COVISE_MESSAGE_QUIT, "");
                //cerr << "Map Editor ____ Close connection\n" << endl;
                ce->accept();
            }
            // wait until user has saved the network
            else
            {
                m_waitForClose = true;
                saveAsNet();
                ce->ignore();
            }
        }else if(msgBox.clickedButton() == quit)
        {
            storeCategoryExpandedState();
            //cerr << "Map Editor ____ Close connection\n" << endl;
            ce->accept();
            m_messageHandler.sendMessage(covise::COVISE_MESSAGE_QUIT, "");
        }
    }

    else
    {
        if (!m_mapEditor.hasMiniGUI())
        {
            storeCategoryExpandedState();
        }
        //cerr << "Map Editor ____ Close connection\n" << endl;
        ce->accept();
        m_messageHandler.sendMessage(covise::COVISE_MESSAGE_QUIT, "");
    }
}

//!
//! quit MapEditor
//!
void MEMainHandler::quit()
{
    m_mapEditor.quit();
    m_quitFunc();
    
}

//!
// !save current module network as picture
//!
void MEMainHandler::printCB()
{
    // store picture
    QString name = "unknown.png";
    if (!m_mapName.isEmpty())
        name = m_mapName.section(".", 0, 0) + ".png";

#if 0
   // get bounding box
   QRectF ff = MEGraphicsView::instance()->scene()->itemsBoundingRect();
   int x1 = (int)ff.x() - 10;
   int y1 = (int)ff.y() - 10;
   int dx = (int)ff.width()  + 10;
   int dy = (int)ff.height() + 10;

   // grab widget pixmap
   //QPixmap p = MEGraphicsView::instance()->viewport()->grab(QRect(x1, y1, dx, dy));
   QPixmap p = MEGraphicsView::instance()->viewport()->grab();
   if (p.isNull() || p.width()==0 || p.height()==0)
   {
      m_mapEditor.printMessage("Failed to grab picture");
      return;
   }

   if(p.save(name, "PNG" ))
      m_mapEditor.printMessage(QString("Picture was stored as %1 (%2x%3 pixels)").arg(name, QString::number(p.width()), QString::number(p.height())));
   else
      m_mapEditor.printMessage("Failed to store picture as " + name);
#else
    QGraphicsScene *scene = MEGraphicsView::instance()->scene();
    scene->setSceneRect(scene->itemsBoundingRect()); // Re-shrink the scene to it's bounding contents
    QImage image(scene->sceneRect().size().toSize(), QImage::Format_ARGB32); // Create the image with the exact size of the shrunk scene
    image.fill(Qt::transparent); // Start all pixels transparent

    QPainter painter(&image);
    scene->render(&painter);
    if (image.save(name))
        m_mapEditor.printMessage(QString("Picture was stored as %1 (%2x%3 pixels)").arg(name, QString::number(image.width()), QString::number(image.height())));
    else
        m_mapEditor.printMessage("Failed to store picture as " + name);
#endif
}

void MEMainHandler::about()
{
    using covise::CoviseVersion;

    QDialog *aboutDialog = new QDialog;
    Ui::MEAboutDialog *ui = new Ui::MEAboutDialog;
    ui->setupUi(aboutDialog);

    QFile file(":/aboutData/README-3rd-party.txt");
    if (file.open(QIODevice::ReadOnly))
    {
        QString data(file.readAll());
        ui->textEdit->setPlainText(data);
    }

    QString text("This is <a href='http://www.hlrs.de/about-us/organization/divisions-departments/av/vis/'>COVISE</a> version %1.%2-<a href='https://github.com/hlrs-vis/covise/commit/%3'>%3</a> compiled on %4 for %5.");
    text = text.arg(CoviseVersion::year())
        .arg(CoviseVersion::month())
        .arg(CoviseVersion::hash())
        .arg(CoviseVersion::compileDate())
        .arg(CoviseVersion::arch());
    text.append("<br>Follow COVISE developement on <a href='https://github.com/hlrs-vis/covise'>GitHub</a>!");

    ui->label->setText(text);
    ui->label->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::LinksAccessibleByMouse);
    ui->label->setOpenExternalLinks(true);

    aboutDialog->exec();
}

void MEMainHandler::aboutQt()
{
    QMessageBox::aboutQt(new QDialog, "Qt");
}

void MEMainHandler::enableExecution(bool state)
{
    if (m_masterUI)
        m_mapEditor.enableExecution(state);
}

//!
//! remove all nodes and clear all lists
//!
void MEMainHandler::clearNet()
{
    // if canvas is not empty and not saved ask if the user will really do that
    if (m_loadedMapWasModified && !MENodeListHandler::instance()->isListEmpty())
    {
        QMessageBox deleteMsgBox(QMessageBox::Question, "Map Editor", "All modules will be deleted. Do you want to continue?");
        auto deleteBtn = deleteMsgBox.addButton("Delete", QMessageBox::ApplyRole);
        auto cancel = deleteMsgBox.addButton(QMessageBox::Cancel);
        deleteMsgBox.exec();
        if(deleteMsgBox.clickedButton() == deleteBtn)
            m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, "NEW_ALL\n");
    }
    // reset handler & userinterface
    else
    {
        m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, "NEW_ALL\n");
    }
}

//!
//! delete all nodes and reset depending stuff!
//!
void MEMainHandler::reset()
{
    m_mapName = "";
    setMapModified(false);
    m_autoSave = false;
    updateLoadedMapname("");

    m_mapEditor.reset();
    MELinkListHandler::instance()->clearList();
    MENodeListHandler::instance()->clearList();
    enableExecution(false);

    MEGraphicsView::instance()->viewAll();
}

//!
//! return the current host color
//!
QColor MEMainHandler::getHostColor(int entry)
{
    QColor color;

    color.setNamedColor("ivory");
    if ((unsigned)entry < (unsigned int)cfg_HostColors->size())
    {
        std::string colorName = (*cfg_HostColors)[entry];
        color.setNamedColor(colorName.c_str());
    }

    return color;
}

void MEMainHandler::updateRemotePartners(const covise::ClientList &partners){
    std::unique_lock<std::mutex> g{m_remotePartnerMutex};
    m_remotePartners = partners;
    if (m_addPartnerDialog)
    {
        m_addPartnerDialog->setPartners(m_remotePartners);
    }
    
}

//!
//! return the distance between two ports
//!
int MEMainHandler::getGridSize()
{
    int dx = (int)MENode::getDistance();
    int gridsize = cfg_GridSize->value() * (m_portSize);
    int xstart = gridsize + dx;

    return xstart;
}

//!
//! update the loaded map name
//!
void MEMainHandler::updateLoadedMapname(const QString &name)
{
    m_mapEditor.setWindowFilePath(name);
    m_mapEditor.setWindowTitle(generateTitle(name));
    storeMapName(name);
}

//!
//! update the parameter value, connected to player, sequencer or timer
//!
void MEMainHandler::updateTimer()
{
    // loop over all timer
    foreach (METimer *tptr, timerList)
    {
        // is timer active
        if (tptr->isActive())
        {

            // play mode --> update value and execute
            if (tptr->getAction() == METimer::PLAY)
                tptr->getPort()->playCB();

            else if (tptr->getAction() == METimer::REVERSE)
                tptr->getPort()->reverseCB();
        }
    }
}

//!
//! return the current config
//!
covise::config::File &MEMainHandler::getConfig()
{
    return m_mapEditorConfig;
}

QSettings &MEMainHandler::getUserBehaviour() //registry entry that stores usage parameteters and is not meant to be modified manually
{
    return m_guiSettings;
}

//!
//! open a dialog window for setting of session parameter
//!
void MEMainHandler::settingXML()
{
    if (!m_settings)
        m_settings = new MESessionSettings(0);

    if (m_settings->exec() == QDialog::Accepted)
    {
        m_mapEditorConfig.save();
        s_highlightColor.setNamedColor(cfg_HighColor->value().c_str());

        if (m_autoSaveTimer)
        {
            m_autoSaveTimer->stop();
            m_autoSaveTimer->start(cfg_AutoSaveTime->value() * 1000);
        }

        // set layout style
        if (m_cfg_QtStyle.hasValidValue() && !cfg_QtStyle().isEmpty())
        {
            QStyle *s = QStyleFactory::create(cfg_QtStyle());
            if (s)
                QApplication::setStyle(s);
        }
        MEGraphicsView::instance()->update();
    }
}

//!
//! add a new host (called from covise)
//!
void MEMainHandler::initHost(int clientId, const covise::UserInfo &partnerInfo, const std::vector<std::string> &modules, const std::vector<std::string> &categories)
{
    MEHost *host = new MEHost(clientId, partnerInfo);
    host->addHostItems(modules, categories);

    addNewHost(host);
}

//!
//! add a new host, reset choose boxes
//!
void MEMainHandler::addNewHost(MEHost *host)
{

    // update hostlist handler
    MEHostListHandler::instance()->addHost(host);
    int activeHosts = MEHostListHandler::instance()->getNoOfHosts();

    // fill the module tree
    // take information from host, category & module
    m_mapEditor.addHostToModuleTree(host);

    if (m_mirrorBox)
    {
        delete m_mirrorBox;
        m_mirrorBox = NULL;
    }

    // update list of possible hosts
    MEGraphicsView::instance()->addHost(host);

    // change items in node popupmenu if only one host is active
    if (activeHosts == 1)
        MEGraphicsView::instance()->setSingleHostNodePopup();

    else
        MEGraphicsView::instance()->setMultiHostNodePopup();

    // enable delete host item in menu bar and master request in toolbar
    m_mapEditor.setCollabItems(activeHosts, m_masterUI);
}

//!
//! get the master state
//!
int MEMainHandler::isMaster()
{
    return m_masterUI;
}

//!
//! show help for certain module
//!
void MEMainHandler::showModuleHelp(const QString &category, const QString &module)
{

    QString moduleHelpFile;

    if (category == "All")
    {
        if (module.isEmpty() || module == "More...")
        {
            moduleHelpFile = onlineDir + "/modules/index.html";
        }
        else
        {
            QString cat = module.section((char)' ', 1L, 1L).section((char)'(', 1L, 1L).section((char)')', 0L, 0L);
            QString mod = module.section((char)' ', 0L, 0L);
            moduleHelpFile = onlineDir + "/modules/" + cat + "/" + mod + "/" + mod + ".html";
        }
    }

    else
    {
        if (module.isEmpty() || module == "More...")
        {
            moduleHelpFile = onlineDir + "/modules/" + category + "/" + "index.html";
        }
        else
        {
            moduleHelpFile = onlineDir + "/modules/" + category + "/" + module + "/" + module + ".html";
        }
    }

    if (MEMainHandler::instance()->isThereAWebHelp())
    {
        if (QUrl(moduleHelpFile).isValid())
            m_helpViewer->newSource(moduleHelpFile);

        else
            QMessageBox::warning(0, "Help Error", "No help available for module");
    }

    else
    {
        QDir d(moduleHelpFile);
        if (d.exists(moduleHelpFile))
            m_helpViewer->newSource(moduleHelpFile);

        else
            QMessageBox::warning(0, "Help Error", "No help available for module");
    }
}

//!
//! remove all nodes for a given host, clear lists and selections
//!
void MEMainHandler::removeNodesOfHost(MEHost *host)
{
    QList<MENode *> list = MENodeListHandler::instance()->getNodesForHost(host);

    foreach (MENode *node, list)
        removeNode(node);

    mapWasChanged("UI_REMOVE_NODES");
}


//!
//! init a module node
//!
void MEMainHandler::initNode(const QStringList &list)
{

    // no "exec on change" during initialisation
    int save_exec = m_executeOnChange;
    m_mapEditor.changeExecButton(false);

    // create a new node
    m_newNode = MENodeListHandler::instance()->addNode(MEGraphicsView::instance());
    int x = (int)(list[4].toInt() * MEGraphicsView::instance()->positionScaleX());
    int y = (int)(list[5].toInt() * MEGraphicsView::instance()->positionScaleY());
    m_newNode->init(list[1], list[2], list[3], x, y);

    // execution is possible
    enableExecution(true);

    // automatically request a synced node, if host of current node has mirrors
    // and the mirror mode is started
    /*if(m_masterUI && m_mirrorMode >= 2)
   {
      int cnt = 0;
      MEHost *host = m_newNode->getHost();
      foreach ( MEHost *mirror, host->mirrorNames)
      {
         requestNode(mname, mirror->getIPAddress(), x+(cnt+1)*200, y, m_currentNode, mapEditor::SYNC);
         cnt++;
      }

      if(host->mirrorNames.count() == 0)  // copy on the same host for mirrored pipeline
      {
         requestNode(mname, hname, x+(cnt+1)*200, y, m_currentNode, mapEditor::SYNC);
      }
   }*/

    // reset mode
    m_mapEditor.changeExecButton(save_exec);

    mapWasChanged("UI_INIT");
}

//!
//! draw new connection lines after moving nodes
//!
void MEMainHandler::moveNode(MENode *node, int x, int y)
{
    node->moveNode((int)(x * MEGraphicsView::instance()->positionScaleX()), (int)(y * MEGraphicsView::instance()->positionScaleY()));
    MEGraphicsView::instance()->ensureVisible(node);
    MELinkListHandler::instance()->resetLinks(node);

    mapWasChanged("UI_NODE_MOVE");
}

//!
//! module node has started, highlight node
//!
void MEMainHandler::startNode(const QString &module, const QString &number, const QString &host)
{
    MENode *node = MENodeListHandler::instance()->getNode(module, number, host);
    if (node != NULL)
        node->setActive(true);
}

//!
//! module node has finished, reset node and update data object names
//!-
void MEMainHandler::finishNode(const QStringList &list)
{
    MENode *node = MENodeListHandler::instance()->getNode(list[0], list[1], list[2]);
    if (node != NULL)
    {
        node->setActive(false);

        // start pointer in message list
        int it = 3;

        // update scalar parameters only
        // no. of parameter sets (not used anymore, is always one)
        int nset = list[it].toInt();
        it++;

        // update data object names & tooltips
        nset = list[it].toInt();
        it++;
        for (int i = 0; i < nset; i++)
        {
            MEDataPort *port = node->getDataPort(list[it]);
            if (port != NULL)
            {
                // update content in data viewr
                port->updateDataObjectNames(list[it + 1]);

                // update help text
                port->setHelpText();

                // update tooltip of "to port" (input port)
                // used data type is known after executing the module
                MELinkListHandler::instance()->updateInputDataPorts(port);
            }
            it = it + 2;
        }
    }
}

//!
//! remove a node, clear lists and selections
//!
void MEMainHandler::removeNode(MENode *node)
{
    MELinkListHandler::instance()->removeLinks(node);
    MENodeListHandler::instance()->removeNode(node);
}

//!
//! delete a host, all nodes & depending stuff
//!
void MEMainHandler::removeHost(MEHost *host)
{

    MEGraphicsView::instance()->removeHost(host);
    MEHostListHandler::instance()->removeHost(host);
    int activeHosts = MEHostListHandler::instance()->getNoOfHosts();

    // deactivate chat window if only one host is active
    // reactivate message line


    // reset menu and tool bar
    m_mapEditor.setCollabItems(activeHosts, m_masterUI);

    // reset node popup action
    if (activeHosts == 1)
        MEGraphicsView::instance()->setSingleHostNodePopup();

    mapWasChanged("UI_DEL_HOST");
}

//!
//! connect two data ports
//!
void MEMainHandler::addLink(MENode *n1, MEPort *p1, MENode *n2, MEPort *p2)
{
    MELinkListHandler::instance()->addLink(n1, p1, n2, p2);
    mapWasChanged("UI_ADD_LINK");
}

//!
//! disconnect to data ports
//!
void MEMainHandler::removeLink(MENode *n1, MEPort *p1, MENode *n2, MEPort *p2)
{
    MELinkListHandler::instance()->deleteLink(n1, p1, n2, p2);
    mapWasChanged("UI_REMOVE_LINK");
}

//!
//! request the master state
//!
void MEMainHandler::masterCB()
{
    QString data;
    QStringList list;

    m_requestingMaster = true;

    list << "MASTERREQ" << localIP << localUser;
    data = list.join("\n");
    m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, data);
    list.clear();
}

//!
//! set the selected host in textfield
//!
void MEMainHandler::getHostCB(QListWidgetItem *item)
{
    if (item)
        m_selectHostLine->setText(item->text());
    else
        m_selectHostLine->setText("");
}

//!
//! set state if user interface loads a map
//!
void MEMainHandler::setInMapLoading(bool state)
{
    m_inMapLoading = state;

    // stop autosave map reload
    if (m_inMapLoading)
    {
    }
}

//!
//! show the state of hosts in moduletree
//!
void MEMainHandler::showHostState(const QStringList &list)
{
    m_connectedPartner = 0;
    int ie = 1;
    int size = (int)(list[ie].toInt());
    ie++;

    for (int i = 0; i < size; i++)
    {
        QString mode = list[ie];
        ie++;
        QString hostname = list[ie];
        ie++;
        QString username = list[ie];
        ie++;
        QString state = list[ie];
        ie++;
        MEHost *host = MEHostListHandler::instance()->getHost(hostname, username);
        if (host)
        {
            QTreeWidgetItem *root = host->getModuleRoot();
            if (mode == "COHOST")
                root->setIcon(0, pm_addhost);

            else if (mode == "COPARTNER")
            {
                m_connectedPartner++;
                if (state == "MASTER")
                    root->setIcon(0, pm_master);
                else
                    root->setIcon(0, pm_adduser);
            }
        }
    }
    if (m_connectedPartner > 1)
        m_mapEditor.openChatWindow();
}

//!
//! enable/disable menu, toolbars ... for the change of master state
//!
void MEMainHandler::setMaster(bool state)
{
    if (state == false && m_masterUI == false && m_requestingMaster)
    {
        QMessageBox::information(NULL,
                                 framework + " Map Editor - Master denied",
                                 "Your request to become the session master was denied.");
    }

    m_requestingMaster = false;

    // set new status
    m_masterUI = state;

    // change action state if more than one host is available
    m_mapEditor.switchMasterState(state);
}

//!
//! check if current host is the first host or if modules of this host are currently used
//!
bool MEMainHandler::canQuitSession()
{
    if (MEHostListHandler::instance()->isListEmpty())
        return true;

    MEHost *host = MEHostListHandler::instance()->getFirstHost();
    if (localHost == host->getHostname())
        return false;

    bool exist = MENodeListHandler::instance()->nodesForHost(localHost);
    return !exist;
}

//!
//! process DESC message
//!
void MEMainHandler::setDescriptionOfNode(const QStringList &list)
{
    if (m_newNode != NULL)
        m_newNode->createPorts(list);

    mapWasChanged("UI_SET_DESC");

    // update scene rectangle
    MEGraphicsView::instance()->updateSceneRect();
}

//!
//! select all nodes created from a clipboard buffer
//!
void MEMainHandler::showClipboardNodes(const QStringList &list)
{
    MEGraphicsView::instance()->scene()->clearSelection();
    int no = list[1].toInt();
    int ie = 2;
    for (int i = 0; i < no; i++)
    {
        MENode *node = MENodeListHandler::instance()->getNode(list[ie], list[ie + 1], list[ie + 2]);
        if (node != NULL)
            node->setSelected(true);
        ie = ie + 3;
    }
}

//!
//! request a node from controller
//!
void MEMainHandler::requestNode(const QString &modulename, const QString &HostIP,
                                int x, int y, MENode *m_clonedNode, MEMainHandler::copyModes mode)
{

    QString cx, cy;
    QStringList buffer;

    if (mode == NORMAL)
        buffer << "INIT";

    else if (mode == SYNC)
        buffer << "SYNC";

    else if (mode == DEBUG)
        buffer << "INIT_DEBUG";

    else if (mode == MEMCHECK)
        buffer << "INIT_MEMCHECK";

    x = (int)(x / MEGraphicsView::instance()->positionScaleX());
    y = (int)(y / MEGraphicsView::instance()->positionScaleY());
    cx.setNum(x);
    cy.setNum(y);

    // send a normal INIT
    int key = -1;
    QString hostIP = HostIP;
    if (hostIP.length() == 0)
    {
        hostIP = localIP;
    }
    buffer << modulename << QString::number(key) << hostIP << cx << cy;

    // send copy  or move init
    if (mode == MEMainHandler::SYNC)
    {
        buffer << m_clonedNode->getName() << m_clonedNode->getNumber() << m_clonedNode->getHostname();
        buffer << QString::number(mode);
    }

    QString data = buffer.join("\n");
    m_messageHandler.sendMessage(covise::COVISE_MESSAGE_UI, data);
    buffer.clear();
    mapWasChanged("INIT/SYNC");
}

//!
//! set the first host as local host
//!
void MEMainHandler::setLocalHost(int id, const QString &name, const QString &user)
{
    m_localHostID = id;
    localHost = name;
    localUser = user;
}

void MEMainHandler::developerModeHasChanged()
{
    emit developerMode(cfg_DeveloperMode->value());
}

void MEMainHandler::setMapModified(bool modified)
{
    m_loadedMapWasModified = modified;
     m_mapEditor.setWindowModified(m_loadedMapWasModified);
}

void MEMainHandler::execTriggered()
{
     m_mapEditor.m_errorNumber = 0;
}

bool MEMainHandler::isDeveloperMode() const
{
    return cfg_DeveloperMode->value();
}

void MEMainHandler::setDeveloperMode(bool devMode)
{
    (*cfg_DeveloperMode) = devMode;
}
