#include<mx2-ollama.hpp>

int main(int argc, char **argv) {
    if(argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <host> <model1> <model2> <seed>\n";
        return 1;
    }
    std::string host = argv[1];
    std::string model1 = argv[2];
    std::string model2 = argv[3];
    std::string seed = argv[4];
    mx::ObjectRequest sender1(host, model1);
    mx::ObjectRequest sender2(host, model2);
    sender1.setPrompt(seed);
    bool active = true;
    std::string m1;
    std::string m2;
    size_t colon_pos1 = model1.find(':');
    m1 = (colon_pos1 != std::string::npos) ? model1.substr(0, colon_pos1) : model1;
    size_t colon_pos2 = model2.find(':');
    m2 = (colon_pos2 != std::string::npos) ? model2.substr(0, colon_pos2) : model2;
    while(active) {
        try {
            std::cout << "\n" << m1 << ": ";
            std::string response1 = sender1.generateTextWithCallback([](const std::string &text) {
                std::cout << text;
            });
            sender2.setPrompt(response1);
            std::cout << "\n" << m2 << ": ";
            std::string response2 = sender2.generateTextWithCallback([](const std::string &text) {
                std::cout << text;
            });
            sender1.setPrompt(response2);
        } catch(mx::ObjectRequestException  &e) {
            std::cerr << "Error: " << e.what() << "\n";
            active = false;
            continue;
        }
    }
    return 0;

}