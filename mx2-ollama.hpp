#ifndef __MX2_OLLAMA_HPP__
#define __MX2_OLLAMA_HPP__


#include <exception>
#include <functional>
#include <sstream>
#include <string>

namespace mx {
    enum class Provider {
        Ollama,
        OpenAI,
        Anthropic
    };

    struct ResponseData {
        Provider provider = Provider::Ollama;
        std::string response;
        std::string pending;
        std::string eventData;
        std::string error;
        std::ostringstream stream;
        std::function<void(const std::string&)> callback = nullptr;
    };

    class ObjectRequestException : public std::exception {
    public:
        explicit ObjectRequestException(const std::string &message) : msg(message) {}
        virtual const char* what() const noexcept override {
            return msg.c_str();
        }
    private:
        std::string msg;
    };  

    class ObjectRequest {
    public:

        explicit ObjectRequest(const std::string &host_ = "localhost", const std::string &model_ = "codellama:7b");
        ObjectRequest(Provider provider_, const std::string& model_,
                      const std::string& host_ = "");
        void setHost(const std::string &host_) {
            host = host_;
        }
        void setModel(const std::string &model_) {
            model = model_;
        }
        void setPrompt(const std::string &prompt_) {
            prompt = prompt_;
        }
        void setMaxTokens(unsigned int maxTokens_) {
            maxTokens = maxTokens_;
        }
        std::string generateText();
        std::string generateTextWithCallback(std::function<void(const std::string&)> callback);
        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, ResponseData* data);
    private:
        Provider provider;
        std::string host;
        std::string model;
        std::string prompt;
        unsigned int maxTokens = 1024;
        std::function<void(const std::string&)> cb = nullptr;
    };



}

#endif
