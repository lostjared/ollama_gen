# ollama_gen
Query a prompt using Ollama, OpenAI, or Anthropic with libcurl and jsoncpp.

To compile, install the libcurl and jsoncpp development packages, then:

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
        std::cout << "\n\nComplete Response: " << response << std::endl;
    } catch (const mx::ObjectRequestException &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }   
    return 0;
}
```

## Cloud providers

Cloud providers use their streaming HTTP APIs and load credentials from the
environment. Set the appropriate variable before running the application:

```bash
export OPENAI_API_KEY="your_openai_api_key"
export ANTHROPIC_API_KEY="your_anthropic_api_key"
```

```cpp
#include <iostream>
#include <mx2-ollama.hpp>

int main() {
    mx::ObjectRequest request(mx::Provider::OpenAI, "your-openai-model");
    request.setPrompt("Hello from C++");
    request.generateTextWithCallback([](const std::string& text) {
        std::cout << text << std::flush;
    });
}
```

Anthropic uses the same interface:

```cpp
mx::ObjectRequest request(
    mx::Provider::Anthropic,
    "your-anthropic-model"
);
request.setMaxTokens(2048);
request.setPrompt("Hello from C++");
std::string response = request.generateText();
```

An optional third constructor argument overrides the provider base URL for a
proxy or compatible endpoint. OpenAI defaults to `https://api.openai.com`,
Anthropic defaults to `https://api.anthropic.com`, and the original Ollama
constructor remains unchanged.
