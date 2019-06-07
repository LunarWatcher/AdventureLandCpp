#include "AdvLand.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace advland {

AdvLandClient::AdvLandClient(std::string email, std::string password)
    : session("adventure.land", 443,
              new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE,
                          9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH")) {
    login(email, password);
}

void AdvLandClient::login(std::string& email, std::string& password) {

    std::stringstream indexStream;
    this->request("/", HTTPRequest::HTTP_GET, indexStream); 
    mLogger->info("Response:\n{}", indexStream.str());
}

void AdvLandClient::request(std::string endpoint, const std::string& method, std::stringstream& out) {
    HTTPRequest connect(method, endpoint, HTTPMessage::HTTP_1_1);
    this->session.sendRequest(connect);

    HTTPResponse response;
    std::istream& responseStream = session.receiveResponse(response);
    if (response.getStatus() != 200) {
        mLogger->warn("Received response code {}");
    }

    Poco::StreamCopier::copyStream(responseStream, out);    
}

} // namespace advland
