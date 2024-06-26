#include "VrbCredentials.h"
#include <config/CoviseConfig.h>
#include <net/covise_host.h>
#include <algorithm>
using namespace vrb;
using namespace covise;

VrbCredentials::VrbCredentials(const std::string &ip, unsigned int tcp, unsigned int udp)
: m_ipAddress(ip)
, m_tcpPort(tcp)
, m_udpPort(udp)
{

}

std::string getIp()
{
    std::string ip = coCoviseConfig::getEntry("System.VRB.Server");
    std::transform(ip.begin(), ip.end(), ip.begin(), [](unsigned char c) { return std::tolower(c); });
    if (ip.empty() || ip == "localhost" || ip == "127.0.0.1")
    {
        ip = Host::getHostaddress();
    }
    return ip;
}

VrbCredentials::VrbCredentials()
    : m_ipAddress(getIp())
    , m_tcpPort(coCoviseConfig::getInt("tcpPort", "System.VRB.Server", 31800))
    , m_udpPort(coCoviseConfig::getInt("udpPort", "System.VRB.Server", 31801))
{
}

const std::string &VrbCredentials::ipAddress() const
{
    return m_ipAddress;
}

unsigned int VrbCredentials::tcpPort() const
{
    return m_tcpPort;
}

unsigned int VrbCredentials::udpPort() const
{
    return m_udpPort;
}


covise::TokenBuffer &vrb::operator<<(covise::TokenBuffer &tb, const VrbCredentials &cred){
    tb << cred.ipAddress() << static_cast<int>(cred.tcpPort()) << static_cast<int>(cred.udpPort());
    return tb;
}

bool vrb::operator==(const VrbCredentials& cred1, const VrbCredentials& cred2){
    return cred1.ipAddress() == cred2.ipAddress() && cred1.tcpPort() == cred2.tcpPort() && cred1.udpPort() == cred2.udpPort();
}
