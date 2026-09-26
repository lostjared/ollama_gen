#include "mx2-ollama.hpp"

#include <curl/curl.h>
#include <json/json.h>

#include <memory>

namespace mx {
    namespace {
        bool processResponseLine(const std::string& line, ResponseData& data) {
            if (line.empty()) {
                return true;
            }

            Json::CharReaderBuilder builder;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            Json::Value object;
            std::string errors;
            if (!reader->parse(line.data(), line.data() + line.size(), &object, &errors)) {
                data.error = "Failed to parse Ollama JSON response: " + errors;
                return false;
            }

            if (object.isMember("error")) {
                data.error = "Ollama API error: " + object["error"].asString();
                return false;
            }

            const Json::Value& response = object["response"];
            if (!response.isNull()) {
                if (!response.isString()) {
                    data.error = "Invalid Ollama JSON response: 'response' is not a string";
                    return false;
                }
                const std::string text = response.asString();
                if (data.callback) {
                    data.callback(text);
                }
                data.stream << text;
            }
            return true;
        }
    }

    size_t ObjectRequest::WriteCallback(void* contents, size_t size, size_t nmemb, ResponseData* data) {
        if (!data) return 0; 
        
        size_t total_size = size * nmemb;
        const std::string chunk(static_cast<char*>(contents), total_size);
        data->response += chunk;
        data->pending += chunk;

        size_t newline;
        while ((newline = data->pending.find('\n')) != std::string::npos) {
            std::string line = data->pending.substr(0, newline);
            data->pending.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!processResponseLine(line, *data)) {
                return 0;
            }
        }

        return total_size;
    }

    std::string ObjectRequest::generateTextWithCallback(std::function<void(const std::string&)> callback) {
        if (host.empty() || model.empty() || prompt.empty()) {
            throw ObjectRequestException("Host, model prompt not set.");
        }
        this->cb = callback;
        std::string response = generateText();   
        return response;
    }

    std::string ObjectRequest::generateText() {
        if (host.empty() || model.empty() || prompt.empty()) {
            throw ObjectRequestException("Host, model prompt not set.");
        }

        Json::Value request(Json::objectValue);
        request["model"] = model;
        request["prompt"] = prompt;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        const std::string json_data = Json::writeString(writer, request);

        
        struct CurlRAII {
            CURL* curl;
            curl_slist* headers;
            
            CurlRAII() : curl(nullptr), headers(nullptr) {
                curl_global_init(CURL_GLOBAL_DEFAULT);
                curl = curl_easy_init();
                if (!curl) {
                    curl_global_cleanup();
                    throw ObjectRequestException("Failed to initialize curl");
                }
            }
            
            ~CurlRAII() {
                if (headers) {
                    curl_slist_free_all(headers);
                }
                if (curl) {
                    curl_easy_cleanup(curl);
                }
                curl_global_cleanup();
            }
            
            CurlRAII(const CurlRAII&) = delete;
            CurlRAII& operator=(const CurlRAII&) = delete;
        };

        CurlRAII curl_raii;
        ResponseData response_data;
        response_data.callback = this->cb;

        if(host.find(":") == std::string::npos) {
            host += ":11434"; 
        }
        std::string url = "http://" + host + "/api/generate";
        curl_easy_setopt(curl_raii.curl, CURLOPT_URL, url.c_str());
        
        curl_easy_setopt(curl_raii.curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl_raii.curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_data.length()));
        
        curl_raii.headers = curl_slist_append(curl_raii.headers, "Content-Type: application/json");
        if (!curl_raii.headers) {
            throw ObjectRequestException("Failed to create HTTP headers");
        }
        curl_easy_setopt(curl_raii.curl, CURLOPT_HTTPHEADER, curl_raii.headers);
        curl_easy_setopt(curl_raii.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_raii.curl, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl_raii.curl, CURLOPT_BUFFERSIZE, 1024L);
        curl_easy_setopt(curl_raii.curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl_raii.curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl_raii.curl, CURLOPT_CONNECTTIMEOUT, 60L);
        curl_easy_setopt(curl_raii.curl, CURLOPT_TIMEOUT, 300L); 
        CURLcode res = curl_easy_perform(curl_raii.curl);
        
        if (res != CURLE_OK) {
            if (!response_data.error.empty()) {
                throw ObjectRequestException(response_data.error);
            }
            throw ObjectRequestException("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
        }

        if (!response_data.pending.empty() &&
            !processResponseLine(response_data.pending, response_data)) {
            throw ObjectRequestException(response_data.error);
        }
        
 
        long response_code;
        curl_easy_getinfo(curl_raii.curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (response_code != 200) {
            throw ObjectRequestException("HTTP request failed with response code: " + std::to_string(response_code) + 
                                       "\nResponse: " + response_data.response);
        }
        
 
        return response_data.stream.str();
    }
}
