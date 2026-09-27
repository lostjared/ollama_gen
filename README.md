# ollama_gen
Query a prompt using Ollama, OpenAI, or Anthropic with libcurl and jsoncpp.

## Building

Install a C++20 compiler and the libcurl, jsoncpp, and pkg-config development
packages first.

### Pcons

With [uv](https://docs.astral.sh/uv/) installed, `uvx` downloads and runs the
Pcons version declared by `pcons-build.py`:

```bash
cd /path/to/ollama_gen
uvx pcons -B build/pcons --reconfigure
```

Run the tests with:

```bash
uvx pcons -B build/pcons test
```

To stage an installation under the repository's `dist` directory:

```bash
uvx pcons -B build/pcons all install
```

For a system or custom prefix, set both the staging prefix and the final prefix
recorded in the installed pkg-config metadata:

```bash
uvx pcons -B build/pcons \
    PCONS_INSTALL_PREFIX=/path/to/prefix \
    PCONS_FINAL_PREFIX=/path/to/prefix \
    all install
```

Use `VARIANT=debug` for a debug build or `TESTS=0` to omit the test program.

### CMake

```bash
cmake -S . -B build/cmake
cmake --build build/cmake
sudo cmake --install build/cmake
```

After installing the library, you can try the example:

```bash
cmake -S example -B example/build
cmake --build example/build
./example/build/example "localhost" "codellama:7b" "Hello "
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
    request.setInstructions("You are a concise coding assistant.");
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
    request.setInstructions("You are a concise coding assistant.");
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
request.setInstructions("You are a concise coding assistant.");
request.setPrompt("Hello from C++");
std::string response = request.generateText();
```

`setInstructions()` sends persistent guidance through each provider's native
system/instructions field. `setPrompt()` contains only the input for the current
request; `ObjectRequest` does not retain conversation history between calls.

An optional third constructor argument overrides the provider base URL for a
proxy or compatible endpoint. OpenAI defaults to `https://api.openai.com`,
Anthropic defaults to `https://api.anthropic.com`, and the original Ollama
constructor remains unchanged.
