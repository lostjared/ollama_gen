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
    std::string result = std::regex_replace(text, think_regex, "<i>$1</i>"); 
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
        file << "<html>\n";
        file << "<head>\n";
        file << "<title>Chat between " << m1 << " and " << m2 << " about: " << seed << "</title>\n";
        file << "<style>\n";
        file << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
        file << ".model1-box { background-color:rgb(127, 177, 224); border: 1px solid:rgb(169, 186, 202); border-radius: 8px; padding: 15px; margin: 10px 0; }\n";
        file << ".model2-box { background-color:rgb(230, 126, 126); border: 1px solid: rgb(162, 117, 117); border-radius: 8px; padding: 15px; margin: 10px 0; }\n";
        file << ".model-name { font-weight: bold; margin-bottom: 8px; }\n";
        file << ".model1-name { color:rgb(7, 24, 42); }\n";
        file << ".model2-name { color:rgb(55, 17, 17); }\n";
        file << "</style>\n";
        file << "</head>\n";
        file << "<body>\n";
        file << "<h1>Chat between " << m1 << " and " << m2 << " about: " << seed << "</h1>\n";
    }

    while(keep_running) {
        try {
            if (!keep_running) break;
            std::cout << "\n" << m1 << ": ";
            if(!filename.empty()) {
                file << "<div class=\"model1-box\">\n";
                file << "<div class=\"model-name model1-name\">" << m1 << ":</div>\n";
            }
            std::string response1 = sender1.generateTextWithCallback([](const std::string &text) {
                std::cout << text;
            });
            if(!filename.empty()) {
                std::string processed = processThinkTags(response1);
                file << newLine(processed) << "<br>";
                file << "</div>\n<br>";
            }   
            if (!keep_running) break;
            sender2.setPrompt(response1);
            std::cout << "\n" << m2 << ": ";
            if(!filename.empty()) {
                file << "<div class=\"model2-box\">\n";
                file << "<div class=\"model-name model2-name\">" << m2 << ":</div>\n";
            }
            std::string response2 = sender2.generateTextWithCallback([](const std::string &text) {
                std::cout << text;
            });
            sender1.setPrompt(response2);
            if(!filename.empty()) {
                std::string processed = processThinkTags(response2);
                file << newLine(processed) << "<br>\n";
                file << "</div>\n<br>";
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
        file << "<div style=\"text-align: center; margin-top: 30px; font-weight: bold; color: #666;\">End of chat</div>\n";
        file << "</body>\n";
        file << "</html>\n";
        file.close();
        std::cout << "\nChat saved to " << filename << "\n";
    }
    std::cout << "Chat ended.\n";
    return 0;
}