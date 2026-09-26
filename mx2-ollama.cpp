#include "mx2-ollama.hpp"

#include <curl/curl.h>
#include <json/json.h>

#include <cstdlib>
#include <memory>
#include <utility>

namespace mx {
    namespace {
        const char* providerName(Provider provider) {
            switch (provider) {
                case Provider::Ollama: return "Ollama";
                case Provider::OpenAI: return "OpenAI";
                case Provider::Anthropic: return "Anthropic";
            }
            return "unknown provider";
        }

        const char* apiKeyEnvironmentVariable(Provider provider) {
            switch (provider) {
                case Provider::OpenAI: return "OPENAI_API_KEY";
                case Provider::Anthropic: return "ANTHROPIC_API_KEY";
                case Provider::Ollama: return nullptr;
            }
            return nullptr;
        }

        std::string defaultHost(Provider provider) {
            switch (provider) {
                case Provider::Ollama: return "localhost";
                case Provider::OpenAI: return "https://api.openai.com";
                case Provider::Anthropic: return "https://api.anthropic.com";
            }
            return {};
        }

        bool parseJson(const std::string& text, Json::Value& object, std::string& error) {
            Json::CharReaderBuilder builder;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            std::string errors;
            if (!reader->parse(text.data(), text.data() + text.size(), &object, &errors)) {
                error = errors;
                return false;
            }
            return true;
        }

        void appendText(const std::string& text, ResponseData& data) {
            if (data.callback) {
                data.callback(text);
            }
            data.stream << text;
        }

        bool processOllamaLine(const std::string& line, ResponseData& data) {
            if (line.empty()) {
                return true;
            }

            Json::Value object;
            std::string errors;
            if (!parseJson(line, object, errors)) {
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
                appendText(response.asString(), data);
            }
            return true;
        }

        std::string jsonErrorMessage(const Json::Value& error) {
            if (error.isString()) {
                return error.asString();
            }
            if (error.isObject() && error["message"].isString()) {
                return error["message"].asString();
            }
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            return Json::writeString(writer, error);
        }

        bool processServerSentEvent(ResponseData& data) {
            if (data.eventData.empty()) {
                return true;
            }

            const std::string event = std::move(data.eventData);
            data.eventData.clear();
            if (event == "[DONE]") {
                return true;
            }

            Json::Value object;
            std::string errors;
            if (!parseJson(event, object, errors)) {
                data.error = "Failed to parse " + std::string(providerName(data.provider)) +
                             " streaming event: " + errors;
                return false;
            }

            const std::string type = object["type"].asString();
            if (type == "error") {
                data.error = std::string(providerName(data.provider)) + " API error: " +
                             jsonErrorMessage(object["error"]);
                return false;
            }

            if (data.provider == Provider::OpenAI) {
                if (type == "response.output_text.delta" && object["delta"].isString()) {
                    appendText(object["delta"].asString(), data);
                } else if (type == "response.failed") {
                    data.error = "OpenAI API error: " +
                                 jsonErrorMessage(object["response"]["error"]);
                    return false;
                }
            } else if (data.provider == Provider::Anthropic &&
                       type == "content_block_delta" &&
                       object["delta"]["type"].asString() == "text_delta" &&
                       object["delta"]["text"].isString()) {
                appendText(object["delta"]["text"].asString(), data);
            }
            return true;
        }

        bool processLine(std::string line, ResponseData& data) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (data.provider == Provider::Ollama) {
                return processOllamaLine(line, data);
            }
            if (line.empty()) {
                return processServerSentEvent(data);
            }
            if (line.starts_with("data:")) {
                std::string value = line.substr(5);
                if (!value.empty() && value.front() == ' ') {
                    value.erase(0, 1);
                }
                if (!data.eventData.empty()) {
                    data.eventData += '\n';
                }
                data.eventData += value;
            }
            return true;
        }

        std::string makeUrl(Provider provider, std::string host) {
            if (host.find("://") == std::string::npos) {
                host = (provider == Provider::Ollama ? "http://" : "https://") + host;
            }
            while (!host.empty() && host.back() == '/') {
                host.pop_back();
            }
            if (provider == Provider::Ollama) {
                const size_t schemeEnd = host.find("://");
                if (host.find(':', schemeEnd + 3) == std::string::npos) {
                    host += ":11434";
                }
                return host + "/api/generate";
            }
            if (provider == Provider::OpenAI) {
                return host + "/v1/responses";
            }
            return host + "/v1/messages";
        }

        std::string makeBody(Provider provider, const std::string& model,
                             const std::string& instructions,
                             const std::string& prompt, unsigned int maxTokens) {
            Json::Value request(Json::objectValue);
            request["model"] = model;
            request["stream"] = true;
            if (provider == Provider::Ollama) {
                if (!instructions.empty()) {
                    request["system"] = instructions;
                }
                request["prompt"] = prompt;
            } else if (provider == Provider::OpenAI) {
                if (!instructions.empty()) {
                    request["instructions"] = instructions;
                }
                request["input"] = prompt;
            } else {
                request["max_tokens"] = maxTokens;
                if (!instructions.empty()) {
                    request["system"] = instructions;
                }
                Json::Value message(Json::objectValue);
                message["role"] = "user";
                message["content"] = prompt;
                request["messages"].append(message);
            }
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            return Json::writeString(writer, request);
        }

        void appendHeader(curl_slist*& headers, const std::string& header) {
            curl_slist* updated = curl_slist_append(headers, header.c_str());
            if (!updated) {
                throw ObjectRequestException("Failed to create HTTP headers");
            }
            headers = updated;
        }

        class CurlGlobal {
          public:
            CurlGlobal() {
                if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
                    throw ObjectRequestException("Failed to initialize curl globally");
                }
            }

            ~CurlGlobal() {
                curl_global_cleanup();
            }

            CurlGlobal(const CurlGlobal&) = delete;
            CurlGlobal& operator=(const CurlGlobal&) = delete;
        };

        CurlGlobal& curlGlobal() {
            static CurlGlobal instance;
            return instance;
        }
    }

    ObjectRequest::ObjectRequest(const std::string& host_, const std::string& model_)
        : provider(Provider::Ollama), host(host_), model(model_) {}

    ObjectRequest::ObjectRequest(Provider provider_, const std::string& model_,
                                 const std::string& host_)
        : provider(provider_),
          host(host_.empty() ? defaultHost(provider_) : host_),
          model(model_) {}

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
            if (!processLine(std::move(line), *data)) {
                return 0;
            }
        }

        return total_size;
    }

    std::string ObjectRequest::generateTextWithCallback(std::function<void(const std::string&)> callback) {
        return generateTextImpl(std::move(callback));
    }

    std::string ObjectRequest::generateText() {
        return generateTextImpl(nullptr);
    }

    std::string ObjectRequest::generateTextImpl(
        std::function<void(const std::string&)> callback) {
        if (host.empty() || model.empty() || prompt.empty()) {
            throw ObjectRequestException("Host, model, or prompt not set.");
        }
        if (provider == Provider::Anthropic && maxTokens == 0) {
            throw ObjectRequestException("Anthropic max tokens must be greater than zero.");
        }

        std::string apiKey;
        if (provider != Provider::Ollama) {
            const char* variable = apiKeyEnvironmentVariable(provider);
            const char* value = std::getenv(variable);
            if (!value || !*value) {
                throw ObjectRequestException(std::string(variable) + " environment variable not set.");
            }
            apiKey = value;
        }

        const std::string json_data = makeBody(
            provider, model, instructions, prompt, maxTokens);

        
        struct CurlRAII {
            CURL* curl;
            curl_slist* headers;
            
            CurlRAII() : curl(nullptr), headers(nullptr) {
                (void)curlGlobal();
                curl = curl_easy_init();
                if (!curl) {
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
            }
            
            CurlRAII(const CurlRAII&) = delete;
            CurlRAII& operator=(const CurlRAII&) = delete;
        };

        CurlRAII curl_raii;
        ResponseData response_data;
        response_data.provider = provider;
        response_data.callback = std::move(callback);

        const std::string url = makeUrl(provider, host);
        curl_easy_setopt(curl_raii.curl, CURLOPT_URL, url.c_str());
        
        curl_easy_setopt(curl_raii.curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl_raii.curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_data.length()));
        
        appendHeader(curl_raii.headers, "Content-Type: application/json");
        if (provider == Provider::OpenAI) {
            appendHeader(curl_raii.headers, "Accept: text/event-stream");
            appendHeader(curl_raii.headers, "Authorization: Bearer " + apiKey);
        } else if (provider == Provider::Anthropic) {
            appendHeader(curl_raii.headers, "Accept: text/event-stream");
            appendHeader(curl_raii.headers, "Authorization: Bearer " + apiKey);
            appendHeader(curl_raii.headers, "anthropic-version: 2023-06-01");
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

        if (!response_data.pending.empty()) {
            if (!processLine(std::move(response_data.pending), response_data)) {
                throw ObjectRequestException(response_data.error);
            }
        }
        if (provider != Provider::Ollama && !processServerSentEvent(response_data)) {
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
