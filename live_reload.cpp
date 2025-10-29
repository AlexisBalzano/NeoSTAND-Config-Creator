#include "live_reload.h"
#include "utils.h"
#include <atomic>
#include <thread>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

struct Impl {
public:
    std::atomic<bool> running{false};
    std::thread serverThread;
    std::thread watchThread;
    std::string mapFilePath;
    std::filesystem::file_time_type lastModified;
    int port = 4000;
};

std::unique_ptr<LiveReloadServer> g_liveServer = nullptr;

// single impl instance used by the wrapper methods
static Impl impl;

LiveReloadServer::LiveReloadServer() {
}

LiveReloadServer::~LiveReloadServer() {
    stop();
}

void LiveReloadServer::startServer(const std::string& mapFile) {
    if (impl.running.load()) return;

    impl.mapFilePath = mapFile;
    if (std::filesystem::exists(mapFile)) {
        impl.lastModified = std::filesystem::last_write_time(mapFile);
    }
    impl.running.store(true);

    // server thread
    impl.serverThread = std::thread([]() {
        std::string baseDir = std::filesystem::path(impl.mapFilePath).parent_path().string();
#ifdef _WIN32
        std::string command = "cd \"" + baseDir + "\" && py -m http.server " + std::to_string(impl.port) + " 2>nul";
#else
        std::string command = "cd \"" + baseDir + "\" && python -m http.server " + std::to_string(impl.port) + " 2>nul";
#endif
        std::system(command.c_str());
    });

    // watch thread
    impl.watchThread = std::thread([]() {
        while (impl.running.load()) {
            if (!impl.mapFilePath.empty() && std::filesystem::exists(impl.mapFilePath)) {
                auto currentModified = std::filesystem::last_write_time(impl.mapFilePath);
                if (currentModified > impl.lastModified) {
                    impl.lastModified = currentModified;
                    std::string signalFile = std::filesystem::path(impl.mapFilePath).parent_path().string() + "/reload_signal.txt";
                    std::ofstream signal(signalFile);
                    if (signal) {
                        signal << std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
                        signal.close();
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    std::cout << GREEN << "Live reload server started at http://localhost:" << impl.port << RESET << std::endl;
}

void LiveReloadServer::stop() {
    if (!impl.running.load()) return;
    impl.running.store(false);
    if (impl.watchThread.joinable()) impl.watchThread.join();
    // serverThread runs python -m http.server which blocks; we cannot forcibly kill it portably here.
    // Leaving it detached is acceptable; it will terminate when process exits.
    if (impl.serverThread.joinable()) {
        impl.serverThread.detach();
    }
}

int LiveReloadServer::getPort() const {
    return impl.port;
}
