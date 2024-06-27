#include "graphql.h"
#include "HTTPClient/CURL/methods.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace graphql {
GraphQLClient::GraphQLClient(const ServerInfo &server, const std::string &schemaPath,
                             const std::string &_endpoint = "/graphql")
: server(server), endpoint(_endpoint)
{
    //TODO: use sax interface of nlohmann::json to parse the schema file => for testing purposes load directly
    std::ifstream schemaFile(schemaPath);
    if (!schemaFile.is_open()) {
        throw std::runtime_error("Could not open schema file: " + schemaPath);
    }
    schema = json::parse(schemaFile);
    // std::cout << "Schema loaded: " << schema.dump(4) << std::endl;
}

// used for testing GET
// void GraphQLClient::send()
// {
//     std::string url = server.url + ":" + std::to_string(server.port) + endpoint;
//     std::string queryStr = "{users{name}}";

//     url += "?query=" + queryStr;
//     std::string response = "";
//     opencover::httpclient::curl::GET get(url);

//     std::cout << get.to_string() << "\n";
//     if (curlRequest.httpRequest(get, response))
//         std::cout << "Response: " << response << std::endl;
// }

void GraphQLClient::send(const std::string &queryStr, const json &variables, std::string &response)
{
    std::string url = server.url + ":" + std::to_string(server.port) + endpoint;

    // Create a JSON object to hold the query and variables
    json requestBody = {{"query", json::parse(queryStr)}};
    requestBody["variables"] = variables;

    // Convert the JSON object to a string
    opencover::httpclient::curl::POST post(url, requestBody.dump());

    // Send the request
    std::cout << post.to_string() << "\n";
    if (curlRequest.httpRequest(post, response))
        std::cout << "Response: " << response << std::endl;
}
} // namespace graphql