#pragma once

#include "platform.h"
#include "util/url.h"

#include <unordered_map>

namespace Tangram {

class MockPlatform : public Platform {

public:
    MockPlatform();

    void shutdown() override {}
    void requestRender() const override;
    std::vector<FontSourceHandle> systemFontFallbacksHandle() const override;
    UrlRequestId startUrlRequest(Url _url, UrlRequestHandle _request) override;
    void urlRequestCanceled(UrlRequestHandle _request) override;

    // Put content at a URL to be retrieved by startUrlRequest.
    void putMockUrlContents(Url url, std::string contents);
    void putMockUrlContents(Url url, std::vector<char> contents);


    // Get the contents of a local file (not mock URL contents).
    static std::vector<char> getBytesFromFile(const char* path);

    Url resolveUrl(Url _url) { return _url.resolved(m_baseUrl); }
private:

    std::unordered_map<Url, std::vector<char>> m_files;
    Url m_baseUrl;
};

} // namespace Tangram
