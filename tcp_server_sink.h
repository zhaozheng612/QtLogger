#pragma once

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/os.h>

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>

#pragma comment(lib, "Ws2_32.lib")

namespace spdlog {
namespace sinks {

template <typename Mutex>
class tcp_server_sink : public base_sink<std::mutex> {
public:
    tcp_server_sink(const std::string &host, int port)
        : host_(host), port_(port), stop_flag_(false) {
        init_winsock_();
        server_thread_ = std::thread(&tcp_server_sink::run_server, this);
    }

    ~tcp_server_sink() {
        stop_flag_ = true;
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        WSACleanup();
    }

protected:
    void sink_it_(const details::log_msg &msg) override {
        memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);
        std::string log_message = fmt::to_string(formatted);

        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto it = clients_.begin(); it != clients_.end(); ) {
            try {
                send(*it, log_message.c_str(), log_message.size());
                ++it;
            } catch (const std::exception &ex) {
                // Handle client send error by removing the client
                it = clients_.erase(it);
            }
        }
    }

    void flush_() override {
        // Implement flushing logic if needed
    }

private:
    void run_server() {
        SOCKET listen_socket = create_listen_socket_();
        while (!stop_flag_) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(listen_socket, &read_fds);
            timeval timeout = {1, 0};  // 1 second timeout

            int rv = select(0, &read_fds, nullptr, nullptr, &timeout);
            if (rv == SOCKET_ERROR) {
                continue;
            }

            if (FD_ISSET(listen_socket, &read_fds)) {
                SOCKET client_socket = accept(listen_socket, nullptr, nullptr);
                if (client_socket != INVALID_SOCKET) {
                    std::lock_guard<std::mutex> lock(clients_mutex_);
                    clients_.emplace_back(client_socket);
                }
            }
        }

        closesocket(listen_socket);
    }

    SOCKET create_listen_socket_() {
        struct addrinfo hints{}, *addrinfo_result = nullptr;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        auto port_str = std::to_string(port_);
        int rv = getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &addrinfo_result);
        if (rv != 0) {
            throw_spdlog_ex("getaddrinfo failed");
        }

        SOCKET listen_socket = socket(addrinfo_result->ai_family, addrinfo_result->ai_socktype, addrinfo_result->ai_protocol);
        if (listen_socket == INVALID_SOCKET) {
            freeaddrinfo(addrinfo_result);
            throw_spdlog_ex("socket creation failed");
        }

        rv = bind(listen_socket, addrinfo_result->ai_addr, (int)addrinfo_result->ai_addrlen);
        if (rv == SOCKET_ERROR) {
            freeaddrinfo(addrinfo_result);
            closesocket(listen_socket);
            throw_spdlog_ex("bind failed");
        }

        freeaddrinfo(addrinfo_result);

        rv = listen(listen_socket, SOMAXCONN);
        if (rv == SOCKET_ERROR) {
            closesocket(listen_socket);
            throw_spdlog_ex("listen failed");
        }

        return listen_socket;
    }

    void send(SOCKET client_socket, const char *data, size_t size) {
        size_t bytes_sent = 0;
        while (bytes_sent < size) {
            int rv = ::send(client_socket, data + bytes_sent, static_cast<int>(size - bytes_sent), 0);
            if (rv == SOCKET_ERROR) {
                closesocket(client_socket);
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.erase(std::remove(clients_.begin(), clients_.end(), client_socket), clients_.end());
                throw_spdlog_ex("send failed");
            }
            bytes_sent += rv;
        }
    }

    static void init_winsock_() {
        WSADATA wsaData;
        int rv = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (rv != 0) {
            throw_spdlog_ex("WSAStartup failed");
        }
    }

    std::string host_;
    int port_;
    std::atomic<bool> stop_flag_;
    std::thread server_thread_;
    std::vector<SOCKET> clients_;
    std::mutex clients_mutex_;
};

using tcp_server_sink_mt = tcp_server_sink<std::mutex>;
using tcp_server_sink_st = tcp_server_sink<spdlog::details::null_mutex>;

}  // namespace sinks
}  // namespace spdlog
