# ollama_gen
Query a prompt to an LLM in Ollama using libcurl.

To compile install libcurl then:

```bash
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install .
```

and after you install the lib you can try the example:

```bash
cd example
mkdir build && cd build
cmake --build .
./example "localhost" "codellama:7b" "Hello "
```

An example program

```cpp
#include<mx2-ollama.hpp>

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <host> <model> <prompt>" << std::endl;
        return 1;
    }
    std::string host = argv[1];
    std::string model = argv[2];
    std::string prompt = argv[3];
    mx::ObjectRequest request(host, model);
    request.setPrompt(prompt);
    try {
        std::string response = request.generateTextWithCallback([](const std::string &chunk) {
            std::cout << chunk << std::flush; 
        });
        std::cout << "Complete Response: " << response << std::endl;
    } catch (const mx::ObjectRequestException &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }   
    return 0;
}
````