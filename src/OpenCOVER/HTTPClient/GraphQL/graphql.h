#ifndef HTTPCLIENT_GRAPHQL_GRAPHQL_H
#define HTTPCLIENT_GRAPHQL_GRAPHQL_H

#include <string>
#include <nlohmann/json.hpp>
#include "HTTPClient/CURL/request.h"
#include "core/export.h"
#include "core/type.h"

namespace opencover::httpclient::graphql {
struct GRAPHQLCLIENTEXPORT ServerInfo {
    std::string url;
    int port;
    std::string toString() const { return url + ":" + std::to_string(port); };
};

class GRAPHQLCLIENTEXPORT GraphQLClient {
public:
    explicit GraphQLClient(const ServerInfo &server, const std::string &schemaPath, const std::string &endpoint);
    ~GraphQLClient(){};

    const ServerInfo &getServer() const { return server; };
    template<typename... Types>
    void send(ObjectType<Types...> &type, std::string &response)
    {
        std::string url = server.url + ":" + std::to_string(server.port) + endpoint;
        auto query = type.toString();
        opencover::httpclient::curl::POST post(url, query);

        std::cout << post.to_string() << "\n";
        if (curlRequest.httpRequest(post, response))
            std::cout << "Response: " << response << std::endl;
    };
    void send(const std::string &queryStr, const nlohmann::json &variables, std::string &response);

private:
    ServerInfo server;
    std::string endpoint;
    nlohmann::json schema;
    curl::Request curlRequest;
};
} // namespace opencover::httpclient::graphql

#endif