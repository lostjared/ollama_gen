#include<mx2-ollama.hpp>
#include<fstream>
#include<csignal>
#include<atomic>
#include<regex>

std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n\nReceived Ctrl+C, shutting down gracefully please be patient...\n";
        keep_running = false;
    }
}

std::string newLine(const std::string& text) {
    std::string result = text;
    size_t pos = 0;
    while ((pos = result.find('\n', pos)) != std::string::npos) {
        result.replace(pos, 1, "<br>");
        pos += 4; 
    }
    return result;
}

std::string processThinkTags(const std::string& text) {
    std::regex think_regex(R"(<think>([\s\S]*?)</think>)", std::regex_constants::icase);
    std::string result = std::regex_replace(text, think_regex, "<div class=\"think-text\">💭 $1</div>"); 
    return result;
}


int main(int argc, char **argv) {
    if(argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <host> <model1> <model2> <seed> <filename>\n";
        return 1;
    }
    std::cout << "Press Ctrl+C to stop the chat.\n";
    std::cout << "Be pateint, the models need time to finish their response.\n";
    std::signal(SIGINT, signal_handler);    
    std::string filename;
    if(argc > 5) {
        filename = argv[5];
    }
    std::string host = argv[1];
    std::string model1 = argv[2];
    std::string model2 = argv[3];
    std::string seed = argv[4];
    mx::ObjectRequest sender1(host, model1);
    mx::ObjectRequest sender2(host, model2);
    sender1.setPrompt(seed);
    std::string m1;
    std::string m2;
    size_t colon_pos1 = model1.find(':');
    m1 = (colon_pos1 != std::string::npos) ? model1.substr(0, colon_pos1) : model1;
    size_t colon_pos2 = model2.find(':');
    m2 = (colon_pos2 != std::string::npos) ? model2.substr(0, colon_pos2) : model2;
    std::fstream file;
    if(!filename.empty()) {
        file.open(filename, std::ios::out);
        if(!file.is_open()) {
            std::cerr << "Error opening file: " << filename << "\n";
            return 1;
        }
        file << "<!DOCTYPE html>\n";
        file << "<html lang=\"en\">\n";
        file << "<head>\n";
        file << "<meta charset=\"UTF-8\">\n";
        file << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        file << "<title>Chat: " << m1 << " & " << m2 << "</title>\n";
        file << "<style>\n";
        file << "* { margin: 0; padding: 0; box-sizing: border-box; }\n";
        file << "body {\n";
        file << "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;\n";
        file << "  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
        file << "  min-height: 100vh;\n";
        file << "  padding: 10px;\n";
        file << "}\n";
        file << ".chat-container {\n";
        file << "  max-width: 600px;\n";
        file << "  margin: 0 auto;\n";
        file << "  background: #ffffff;\n";
        file << "  border-radius: 20px;\n";
        file << "  box-shadow: 0 20px 40px rgba(0,0,0,0.1);\n";
        file << "  overflow: hidden;\n";
        file << "  min-height: 90vh;\n";
        file << "}\n";
        file << ".chat-header {\n";
        file << "  background: linear-gradient(135deg, #4a90e2, #357abd);\n";
        file << "  color: white;\n";
        file << "  padding: 20px;\n";
        file << "  text-align: center;\n";
        file << "  box-shadow: 0 2px 10px rgba(0,0,0,0.1);\n";
        file << "}\n";
        file << ".chat-title {\n";
        file << "  font-size: 1.4em;\n";
        file << "  font-weight: 600;\n";
        file << "  margin-bottom: 5px;\n";
        file << "}\n";
        file << ".chat-subtitle {\n";
        file << "  font-size: 0.9em;\n";
        file << "  opacity: 0.9;\n";
        file << "  font-weight: 300;\n";
        file << "}\n";
        file << ".chat-messages {\n";
        file << "  padding: 20px 15px;\n";
        file << "  background: #f8f9fa;\n";
        file << "  min-height: 60vh;\n";
        file << "}\n";
        file << ".message {\n";
        file << "  margin-bottom: 15px;\n";
        file << "  display: flex;\n";
        file << "  align-items: flex-end;\n";
        file << "  animation: slideIn 0.3s ease-out;\n";
        file << "}\n";
        file << ".message.model1 {\n";
        file << "  justify-content: flex-start;\n";
        file << "}\n";
        file << ".message.model2 {\n";
        file << "  justify-content: flex-end;\n";
        file << "}\n";
        file << ".message-bubble {\n";
        file << "  max-width: 75%;\n";
        file << "  padding: 12px 16px;\n";
        file << "  border-radius: 20px;\n";
        file << "  font-size: 0.95em;\n";
        file << "  line-height: 1.4;\n";
        file << "  word-wrap: break-word;\n";
        file << "  position: relative;\n";
        file << "  box-shadow: 0 2px 8px rgba(0,0,0,0.1);\n";
        file << "}\n";
        file << ".model1 .message-bubble {\n";
        file << "  background: #ffffff;\n";
        file << "  color: #333;\n";
        file << "  border-bottom-left-radius: 8px;\n";
        file << "  border: 1px solid #e1e5e9;\n";
        file << "}\n";
        file << ".model2 .message-bubble {\n";
        file << "  background: linear-gradient(135deg, #4a90e2, #357abd);\n";
        file << "  color: #ffffff !important;\n";  // Force white text with !important
        file << "  border-bottom-right-radius: 8px;\n";
        file << "}\n";
        file << ".message-sender {\n";
        file << "  font-size: 0.75em;\n";
        file << "  font-weight: 600;\n";
        file << "  margin-bottom: 6px;\n";
        file << "  text-transform: uppercase;\n";
        file << "  letter-spacing: 0.5px;\n";
        file << "}\n";
        file << ".model1 .message-sender {\n";
        file << "  color: #4a90e2;\n";
        file << "}\n";
        file << ".model2 .message-sender {\n";
        file << "  color: rgba(238, 48, 48, 0.95);\n";
        file << "}\n";
        file << ".avatar {\n";
        file << "  width: 35px;\n";
        file << "  height: 35px;\n";
        file << "  border-radius: 50%;\n";
        file << "  margin: 0 8px;\n";
        file << "  display: flex;\n";
        file << "  align-items: center;\n";
        file << "  justify-content: center;\n";
        file << "  font-weight: bold;\n";
        file << "  font-size: 0.8em;\n";
        file << "  color: white;\n";
        file << "  flex-shrink: 0;\n";
        file << "}\n";
        file << ".model1 .avatar {\n";
        file << "  background: linear-gradient(135deg, #667eea, #764ba2);\n";
        file << "  order: 0;\n";
        file << "}\n";
        file << ".model2 .avatar {\n";
        file << "  background: linear-gradient(135deg, #f093fb, #f5576c);\n";
        file << "  order: 1;\n";
        file << "}\n";
        file << ".message-content {\n";
        file << "  flex: 1;\n";
        file << "}\n";
        file << ".model2 .message-content {\n";
        file << "  order: 0;\n";
        file << "}\n";
        file << ".think-text {\n";
        file << "  font-style: italic;\n";
        file << "  opacity: 0.8;\n";
        file << "  background: rgba(255,255,255,0.1);\n";
        file << "  padding: 8px 12px;\n";
        file << "  border-radius: 12px;\n";
        file << "  margin: 8px 0;\n";
        file << "  border-left: 3px solid rgba(255,255,255,0.3);\n";
        file << "}\n";
        file << ".model1 .think-text {\n";
        file << "  background: rgba(74,144,226,0.1);\n";
        file << "  border-left-color: #4a90e2;\n";
        file << "  color: #666;\n";
        file << "}\n";
        file << ".chat-end {\n";
        file << "  text-align: center;\n";
        file << "  padding: 20px;\n";
        file << "  color: #666;\n";
        file << "  font-style: italic;\n";
        file << "  background: #f0f0f0;\n";
        file << "  border-top: 1px solid #e1e5e9;\n";
        file << "}\n";
        file << "@keyframes slideIn {\n";
        file << "  from {\n";
        file << "    opacity: 0;\n";
        file << "    transform: translateY(20px);\n";
        file << "  }\n";
        file << "  to {\n";
        file << "    opacity: 1;\n";
        file << "    transform: translateY(0);\n";
        file << "  }\n";
        file << "}\n";
        file << "@media (max-width: 600px) {\n";
        file << "  body { padding: 5px; }\n";
        file << "  .chat-container { border-radius: 10px; }\n";
        file << "  .message-bubble { max-width: 85%; }\n";
        file << "  .chat-header { padding: 15px; }\n";
        file << "}\n";
        file << "</style>\n";
        file << "</head>\n";
        file << "<body>\n";
        file << "<div class=\"chat-container\">\n";
        file << "<div class=\"chat-header\">\n";
        file << "<div class=\"chat-title\">" << m1 << " & " << m2 << "</div>\n";
        file << "<div class=\"chat-subtitle\">Topic: " << seed << "</div>\n";
        file << "</div>\n";
        file << "<div class=\"chat-messages\">\n";
    }

    while(keep_running) {
        try {
            if (!keep_running) break;
            std::cout << "\n" << m1 << ": ";
            if(!filename.empty()) {
                file << "<div class=\"message model1\">\n";
                // For model1 avatar
                std::string avatar1 = m1.length() >= 2 ? m1.substr(0, 2) : (m1.length() == 1 ? m1 + "1" : "M1");
                file << "<div class=\"avatar\">" << avatar1 << "</div>\n";
                file << "<div class=\"message-content\">\n";
                file << "<div class=\"message-sender\">" << m1 << "</div>\n";
                file << "<div class=\"message-bubble\">\n";
            }
            std::string response1 = sender1.generateTextWithCallback([](const std::string &text) {
                std::cout << text;
            });
            if(!filename.empty()) {
                std::string processed = processThinkTags(response1);
                processed = newLine(processed);
                file << processed << "\n";
                file << "</div>\n";
                file << "</div>\n";
                file << "</div>\n";
            }   
            if (!keep_running) break;
            sender2.setPrompt(response1);
            std::cout << "\n" << m2 << ": ";
            if(!filename.empty()) {
                file << "<div class=\"message model2\">\n";
                // For model2 avatar  
                std::string avatar2 = m2.length() >= 2 ? m2.substr(0, 2) : (m2.length() == 1 ? m2 + "2" : "M2");
                file << "<div class=\"avatar\">" << avatar2 << "</div>\n";
                file << "<div class=\"message-content\">\n";
                file << "<div class=\"message-sender\">" << m2 << "</div>\n";
                file << "<div class=\"message-bubble\">\n";
            }
            std::string response2 = sender2.generateTextWithCallback([](const std::string &text) {
                std::cout << text;
            });
            sender1.setPrompt(response2);
            if(!filename.empty()) {
                std::string processed = processThinkTags(response2);
                processed = newLine(processed);
                file << processed << "\n";
                file << "</div>\n";
                file << "</div>\n";
                file << "</div>\n";
                file.flush(); 
            } 
        } catch(mx::ObjectRequestException  &e) {
            std::cerr << "Error: " << e.what() << "\n";
            break;
        } catch(const std::exception &e) {
            std::cerr << "Unexpected error: " << e.what() << "\n";
            break;
        }   
    }
    
    if(!filename.empty()) {
        file << "</div>\n";
        file << "<div class=\"chat-end\">💬 End of conversation</div>\n";
        file << "</div>\n";
        file << "</body>\n";
        file << "</html>\n";
        file.close();
        std::cout << "\nChat saved to " << filename << "\n";
    }
    std::cout << "Chat ended.\n";
    return 0;
}