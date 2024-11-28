#include "userinfo.h"
#include <config/CoviseConfig.h>
#include <net/covise_host.h>
#include <net/tokenbuffer.h>


using namespace covise;

namespace covise{
namespace detail{

Program getUserType(covise::TokenBuffer& tb){
    Program t;
    tb >> t;
    return t;
}
std::string getString(covise::TokenBuffer& tb){
    std::string s;
    tb >> s;
    return s;
}

} //detail

} //covise


UserInfo::UserInfo(covise::TokenBuffer &tb)
    : userType(detail::getUserType(tb))
    , userName(detail::getString(tb))
    , ipAdress(detail::getString(tb))
    , hostName(detail::getString(tb))
    , email(detail::getString(tb))
    , url(detail::getString(tb))
    , icon(detail::getString(tb))
    , avatar(detail::getString(tb))
{
}

UserInfo::UserInfo(Program type)
    : userType(type)
    , userName(covise::coCoviseConfig::getEntry("value", "COVER.Collaborative.UserName", covise::Host::getUsername()))
    , ipAdress(covise::Host::getHostaddress())
    , hostName(covise::Host::getHostname())
    , email(covise::coCoviseConfig::getEntry("value", "COVER.Collaborative.Email", "covise-users@listserv.uni-stuttgart.de"))
    , url(covise::coCoviseConfig::getEntry("value", "COVER.Collaborative.URL", "www.hlrs.de/covise"))
    , icon(covise::coCoviseConfig::getEntry("value", "COVER.Collaborative.Icon", "$COVISE_PATH/share/covise/icons/hosts/localhost.obj"))
    , avatar(covise::coCoviseConfig::getEntry("value", "COVER.Collaborative.Avatar", ""))
{

}

TokenBuffer &covise::operator<<(TokenBuffer &tb, const UserInfo &userInfo)
{
    tb << userInfo.userType << userInfo.userName << userInfo.ipAdress << userInfo.hostName << userInfo.email << userInfo.url << userInfo.icon << userInfo.avatar;
    return tb;
}

std::ostream &covise::operator<<(std::ostream &os, const UserInfo &userInfo)
{
    os << "name:     " << userInfo.userName << std::endl;
    os << "ip:       " << userInfo.ipAdress << std::endl;
    os << "hostName: " << userInfo.hostName << std::endl;
    os << "email:    " << userInfo.email << std::endl;
    os << "url:      " << userInfo.url << std::endl;
    os << "type:     " << userInfo.userType << std::endl;
    os << "icon:     " << userInfo.icon << std::endl;
    os << "avatar:     " << userInfo.avatar << std::endl;
    return os;
}


