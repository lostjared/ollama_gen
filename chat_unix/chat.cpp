#include<mx2-ollama.hpp>
#include<fstream>
#include<csignal>
#include<atomic>

std::atomic<bool> keep_running(true);


void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n\nReceived Ctrl+C, shutting down gracefully...\n";
        keep_running = false;
    }
}

int main(int argc, char **argv) {
    if(argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <host> <model1> <model2> <seed>\n";
        return 1;
    }
    std::cout << "Press Ctrl+C to stop the chat.\n";
    std::cout << "Be pateint, the models need time to respond.\n";
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
        file << "<title>Chat between " << m1 << " and " << m2 << "</title>\n";
        file << "</head>\n";
        file << "<body>\n";
        file << "Chat between <b>" << m1 << "</b> and <b>" << m2 << "</b>\n";
    }

    while(keep_running) {
        try {
            if (!keep_running) break;
            std::cout << "\n" << m1 << ": ";
            if(!filename.empty()) {
                file << "<br><b><span style=\"color:blue;\">" << m1 << ":</span></b> ";
            }
            std::string response1 = sender1.generateTextWithCallback([](const std::string &text) {
                std::cout << text;
            });
            if(!filename.empty()) {
                file << response1 << " ";
            }   
            if (!keep_running) break;
            sender2.setPrompt(response1);
            std::cout << "\n" << m2 << ": ";
            if(!filename.empty()) {
                file << "<br><b><span style=\"color:red;\">" << m2 << ":</span></b> ";
            }
            std::string response2 = sender2.generateTextWithCallback([](const std::string &text) {
                std::cout << text;
            });
            sender1.setPrompt(response2);
            if(!filename.empty()) {
                file << response2 << " ";
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
        file << "<br><b>End of chat</b>\n";
        file << "</body>\n";
        file << "</html>\n";
        file.close();
        std::cout << "\nChat saved to " << filename << "\n";
    }
    std::cout << "Chat ended.\n";
    return 0;
}