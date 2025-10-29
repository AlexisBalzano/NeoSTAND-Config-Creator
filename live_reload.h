#pragma once
#include <string>
#include <memory>

class LiveReloadServer;

extern std::unique_ptr<LiveReloadServer> g_liveServer;

class LiveReloadServer {
public:
    LiveReloadServer();
    ~LiveReloadServer();
    void startServer(const std::string& mapFile);
    void stop();
    int getPort() const;
};
