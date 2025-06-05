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
