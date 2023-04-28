/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#define IOMANIPH
// don't include iomanip.h becaus it interferes with qt


#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
//#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#endif

#include "VRBServer.h"
#include <vrb/server/VrbClientList.h>
using std::cerr;
using std::endl;
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/socket.h>
#endif

#include <QDir>
#include <QHostInfo>
#include <QList>
#include <QHostAddress>

#include "gui/VRBapplication.h"
#include "gui/coRegister.h"
#include "VrbUiClientList.h"
#include <vrb/server/VrbClientList.h>
#include <VrbUiMessageHandler.h>
extern ApplicationWindow *mw;


#include <config/CoviseConfig.h>
#include <net/covise_socket.h>
#include <net/covise_connect.h>
#include <net/message_types.h>
#include <util/unixcompat.h>

#include <qtutil/NetHelp.h>
#include <qtutil/FileSysAccess.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtNetwork/qhostinfo.h>
#include <QSocketNotifier>
//#include <QTreeWidget>

#include <vrb/server/VrbClientList.h>
#include <csignal>
#include "gui/VRBapplication.h"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif
//#define MB_DEBUG

vrb::VRBClientList *vrbClients;

using namespace covise;
using namespace vrb;

VRBServer::VRBServer(bool gui)
    :m_gui(gui)
{
    covise::Socket::initialize();
#ifndef _WIN32
    std::signal(SIGUSR1, SIG_IGN); //ignore signal that is used to check if VRB process is running
#endif
    m_tcpPort = coCoviseConfig::getInt("tcpPort", "System.VRB.Server", 31800);
    m_udpPort = coCoviseConfig::getInt("udpPort", "System.VRB.Server", m_tcpPort +1);
    requestToQuit = false;
    if (gui)
    {
        vrbClients = &uiClients;
        handler.reset(new VrbUiMessageHandler(this));
    }
    else
    {
        vrbClients = &clients;
        handler.reset(new VrbMessageHandler(this));
    }
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN); // otherwise writes to a closed socket kill the application.
#endif
}

void VRBServer::removeConnection(const covise::Connection * conn)
{
    connections.remove(conn);
    if (requestToQuit && handler->numberOfClients() == 0)
    {
        exit(0);
    }
}

int VRBServer::getPort()
{
    return m_tcpPort;
}

int VRBServer::getUdpPort()
{
    return m_udpPort;
}

bool VRBServer::openServer(bool printport)
{
    if (printport) {
        for (int port = 1024; port <= 0xffff; ++port) {
            sConn = connections.tryAddNewListeningConn<ServerConnection>(port, 0, 0);
            if (sConn) {
                m_tcpPort = port;
                break;
            }
        }

        if (!sConn) {
            fprintf(stderr, "Could not open server on any port\n");
            return false;
        }
    }
    else
    {
        sConn = connections.tryAddNewListeningConn<ServerConnection>(m_tcpPort,
                                                                     0, 0);
        if (!sConn) {
            fprintf(stderr, "Could not open server port %d\n", m_tcpPort);
            return false;
        }
    }
    if (m_gui)
    {
        serverSN.reset(new QSocketNotifier(sConn->get_id(NULL), QSocketNotifier::Read));
        QObject::connect(&*serverSN, SIGNAL(activated(int)),
            this, SLOT(processMessages()));
    }
    return true;
}

void VRBServer::loop()
{
    while (true)
    {
		processMessages(10.f);
    }
}

void VRBServer::processMessages(float waitTime)
{
	while (const Connection *conn = connections.check_for_input(waitTime))
    {
        if (conn == udpConn) // udp connection
        {
			processUdpMessages();
		}
        else if (conn == sConn) //tcp connection to server port
        {
            addClient();
        }
        else
        {
            if (m_gui)
                static_cast<VrbUiMessageHandler*>(handler.get())->setClientNotifier(conn, false);

            if (conn->recv_msg(&msg))
                handler->handleMessage(&msg);

            if (m_gui)
                static_cast<VrbUiMessageHandler*>(handler.get())->setClientNotifier(conn, true);
        }
    }
}

void VRBServer::addClient()
{
    auto clientConn = sConn->spawn_connection();
    struct linger linger;
    linger.l_onoff = 0;
    linger.l_linger = 0;
    setsockopt(clientConn->get_id(NULL), SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    vrb::ConnectionDetails::ptr cd;
    if (m_gui)
    {
        QSocketNotifier* sn = new QSocketNotifier(clientConn->get_id(NULL), QSocketNotifier::Read);
        QObject::connect(sn, SIGNAL(activated(int)),
            this, SLOT(processMessages()));

        auto uicd = UiConnectionDetails::ptr{ new UiConnectionDetails{} };
        uicd->notifier.reset(sn);
        cd = std::move(uicd);
    }
    else
    {
        cd.reset(new vrb::ConnectionDetails{});
    }
    cd->tcpConn = connections.add(std::move(clientConn));
    cd->udpConn = udpConn;
    handler->addClient(std::move(cd));
    std::cerr << "VRB new client request" << std::endl;
}
void VRBServer::processUdpMessages()
{
	while (udpConn->recv_udp_msg(&udpMsg))
	{
		handler->handleUdpMessage(&udpMsg);
	}
}
bool VRBServer::startUdpServer() 
{
    auto conn = std::unique_ptr<UDPConnection>(new UDPConnection{0, 0, m_udpPort, nullptr});
    if (conn->getSocket()->get_id() < 0)
    {
		return false;
	}
	struct linger linger;
	linger.l_onoff = 0;
	linger.l_linger = 0;
        udpConn = dynamic_cast<const UDPConnection*>(connections.add(std::move(conn)));
	setsockopt(udpConn->get_id(NULL), SOL_SOCKET, SO_LINGER, (char*)& linger, sizeof(linger));
	if (m_gui)
	{
		QSocketNotifier* sn = new QSocketNotifier(udpConn->get_id(NULL), QSocketNotifier::Read);
		QObject::connect(sn, SIGNAL(activated(int)),
			this, SLOT(processUdpMessages()));
	}
	return true;
}
