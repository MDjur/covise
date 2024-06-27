#ifndef _LIB_GRAPHQL_H
#define _LIB_GRAPHQL_H

#include <string>
#include <nlohmann/json.hpp>
#include "HTTPClient/CURL/request.h"


using json = nlohmann::json;

namespace graphql {

struct ServerInfo {
    std::string url;
    int port;
    std::string toString() const { return url + ":" + std::to_string(port); };
};

class GraphQLClient {
public:
    explicit GraphQLClient(const ServerInfo &server, const std::string &schemaPath,
                           const std::string &endpoint);
    ~GraphQLClient(){};

    const ServerInfo &getServer() const { return server; };
    void send();
    void send(const std::string &queryStr, const json &variables, std::string &response);

private:
    ServerInfo server;
    std::string endpoint;
    json schema;
    opencover::httpclient::curl::Request curlRequest;
};
} // namespace graphql

#endif