#include "mx2-ollama.hpp"

#include <cassert>
#include <string>
#include <vector>

namespace {
    size_t writeChunk(const std::string& chunk, mx::ResponseData& data) {
        return mx::ObjectRequest::WriteCallback(
            const_cast<char*>(chunk.data()), 1, chunk.size(), &data);
    }

    void writeInChunks(const std::string& response, mx::ResponseData& data) {
        const std::vector<size_t> chunkSizes = {5, 13, 2, 19, 1, 7, 100};
        size_t offset = 0;
        size_t chunkIndex = 0;
        while (offset < response.size()) {
            const size_t chunkSize = chunkSizes[chunkIndex % chunkSizes.size()];
            const std::string chunk = response.substr(offset, chunkSize);
            assert(writeChunk(chunk, data) == chunk.size());
            offset += chunk.size();
            ++chunkIndex;
        }
        assert(offset == response.size());
    }
}

int main() {
    mx::ResponseData data;
    std::vector<std::string> callbacks;
    data.callback = [&callbacks](const std::string& text) {
        callbacks.push_back(text);
    };

    const std::string response =
        "{\"response\":\"hello \\\"world\\\"\\nSnowman: \\u2603\"}\n"
        "{\"response\":\"!\"}\n"
        "{\"done\":true}\n";

    writeInChunks(response, data);

    assert(data.pending.empty());
    assert(data.error.empty());
    assert(data.response == response);
    assert(data.stream.str() == "hello \"world\"\nSnowman: \xE2\x98\x83!");
    assert(callbacks.size() == 2);
    assert(callbacks[0] == "hello \"world\"\nSnowman: \xE2\x98\x83");
    assert(callbacks[1] == "!");

    mx::ResponseData invalid;
    const std::string malformed = "{not json}\n";
    assert(writeChunk(malformed, invalid) == 0);
    assert(!invalid.error.empty());

    mx::ResponseData openAI;
    openAI.provider = mx::Provider::OpenAI;
    const std::string openAIResponse =
        "event: response.created\n"
        "data: {\"type\":\"response.created\"}\n\n"
        "event: response.output_text.delta\n"
        "data: {\"type\":\"response.output_text.delta\",\"delta\":\"Hello\"}\n\n"
        "event: response.output_text.delta\n"
        "data: {\"type\":\"response.output_text.delta\",\"delta\":\" cloud\"}\n\n";
    writeInChunks(openAIResponse, openAI);
    assert(openAI.pending.empty());
    assert(openAI.error.empty());
    assert(openAI.stream.str() == "Hello cloud");

    mx::ResponseData anthropic;
    anthropic.provider = mx::Provider::Anthropic;
    const std::string anthropicResponse =
        "event: content_block_delta\r\n"
        "data: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"Hello Claude\"}}\r\n\r\n"
        "event: message_stop\r\n"
        "data: {\"type\":\"message_stop\"}\r\n\r\n";
    writeInChunks(anthropicResponse, anthropic);
    assert(anthropic.pending.empty());
    assert(anthropic.error.empty());
    assert(anthropic.stream.str() == "Hello Claude");

    mx::ResponseData cloudError;
    cloudError.provider = mx::Provider::Anthropic;
    const std::string errorResponse =
        "event: error\n"
        "data: {\"type\":\"error\",\"error\":{\"type\":\"overloaded_error\",\"message\":\"Overloaded\"}}\n\n";
    assert(writeChunk(errorResponse, cloudError) == 0);
    assert(cloudError.error == "Anthropic API error: Overloaded");

    mx::ObjectRequest missingKey(mx::Provider::OpenAI, "test-model");
    missingKey.setPrompt("This must fail before making a request");
    try {
        (void)missingKey.generateText();
        assert(false);
    } catch (const mx::ObjectRequestException& error) {
        assert(std::string(error.what()) ==
               "OPENAI_API_KEY environment variable not set.");
    }

    mx::ObjectRequest invalidMaxTokens(mx::Provider::Anthropic, "test-model");
    invalidMaxTokens.setPrompt("This must also fail before making a request");
    invalidMaxTokens.setMaxTokens(0);
    try {
        (void)invalidMaxTokens.generateText();
        assert(false);
    } catch (const mx::ObjectRequestException& error) {
        assert(std::string(error.what()) == "Anthropic max tokens must be greater than zero.");
    }
}
