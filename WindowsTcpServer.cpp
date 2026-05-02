#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "common/Chess.h"

#pragma comment(lib, "ws2_32.lib")

using chess::Board;
using chess::Color;

namespace {

struct Client {
    SOCKET fd = INVALID_SOCKET;
    std::string name = "Player";
};

bool sendLine(SOCKET fd, const std::string& line) {
    std::string data = line;
    if (data.empty() || data.back() != '\n') data.push_back('\n');
    const char* ptr = data.c_str();
    int left = static_cast<int>(data.size());
    while (left > 0) {
        int sent = send(fd, ptr, left, 0);
        if (sent <= 0) return false;
        ptr += sent;
        left -= sent;
    }
    return true;
}

bool recvLine(SOCKET fd, std::string& out) {
    out.clear();
    char ch = 0;
    while (true) {
        int n = recv(fd, &ch, 1, 0);
        if (n <= 0) return false;
        if (ch == '\n') break;
        if (ch != '\r') out.push_back(ch);
        if (out.size() > 2048) return false;
    }
    return true;
}

void closeClient(Client& c) {
    if (c.fd != INVALID_SOCKET) {
        shutdown(c.fd, SD_BOTH);
        closesocket(c.fd);
        c.fd = INVALID_SOCKET;
    }
}

std::string trimNameFromHello(const std::string& line) {
    if (line.rfind("HELLO ", 0) == 0) {
        std::string name = line.substr(6);
        if (!name.empty() && name.size() <= 40) return name;
    }
    return "Player";
}

Client acceptClient(SOCKET listenFd) {
    sockaddr_in addr{};
    int len = sizeof(addr);
    SOCKET fd = accept(listenFd, reinterpret_cast<sockaddr*>(&addr), &len);
    Client c;
    c.fd = fd;
    if (fd == INVALID_SOCKET) return c;

    char ip[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    std::cout << "Yeni baglanti: " << ip << ':' << ntohs(addr.sin_port) << std::endl;

    sendLine(fd, "MSG Windows TCP satranc sunucusuna baglandin. Rakip bekleniyor...");
    std::string hello;
    if (recvLine(fd, hello)) c.name = trimNameFromHello(hello);
    return c;
}

void matchThread(Client white, Client black) {
    std::cout << "Mac basladi: " << white.name << " (Beyaz) vs " << black.name << " (Siyah)" << std::endl;
    Board board;

    sendLine(white.fd, "COLOR W");
    sendLine(black.fd, "COLOR B");
    sendLine(white.fd, "MSG Rakip bulundu: " + black.name);
    sendLine(black.fd, "MSG Rakip bulundu: " + white.name);
    sendLine(white.fd, "START");
    sendLine(black.fd, "START");

    while (true) {
        Client& current = board.sideToMove() == Color::White ? white : black;
        Client& other = board.sideToMove() == Color::White ? black : white;

        std::string line;
        if (!recvLine(current.fd, line)) {
            sendLine(other.fd, "MSG Rakip baglantidan cikti. Oyun bitti.");
            break;
        }
        if (line.rfind("MOVE ", 0) != 0) {
            sendLine(current.fd, "ERR Sadece MOVE komutu bekleniyor.");
            continue;
        }
        std::string uci = line.substr(5);
        if (!board.makeMoveUci(uci)) {
            sendLine(current.fd, "ERR Gecersiz hamle: " + uci);
            continue;
        }
        sendLine(current.fd, "OK " + uci);
        sendLine(other.fd, "MOVE " + uci);
        std::string status = board.statusText();
        if (board.isCheckmate() || board.isStalemate()) {
            sendLine(white.fd, "MSG " + status);
            sendLine(black.fd, "MSG " + status);
            break;
        }
    }
    closeClient(white);
    closeClient(black);
    std::cout << "Mac kapandi." << std::endl;
}

SOCKET createListenSocket(int port) {
    SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == INVALID_SOCKET) return INVALID_SOCKET;

    BOOL yes = TRUE;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(fd);
        return INVALID_SOCKET;
    }
    if (listen(fd, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(fd);
        return INVALID_SOCKET;
    }
    return fd;
}

} // namespace

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "Winsock baslatilamadi." << std::endl;
        return 1;
    }

    int port = 5555;
    if (argc >= 2) {
        std::istringstream ss(argv[1]);
        ss >> port;
        if (port <= 0 || port > 65535) port = 5555;
    }

    SOCKET listenFd = createListenSocket(port);
    if (listenFd == INVALID_SOCKET) {
        std::cerr << "Sunucu baslatilamadi. Port kullanimda olabilir: " << port << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Windows TCP ChessServer calisiyor. Port: " << port << std::endl;
    std::cout << "Ayni ag: istemciler bu bilgisayarin yerel IP adresine baglanir." << std::endl;
    std::cout << "Farkli ag: modem/router uzerinden TCP " << port << " portunu bu bilgisayara yonlendir veya VPS kullan." << std::endl;

    while (true) {
        Client white = acceptClient(listenFd);
        if (white.fd == INVALID_SOCKET) continue;
        Client black = acceptClient(listenFd);
        if (black.fd == INVALID_SOCKET) {
            closeClient(white);
            continue;
        }
        std::thread(matchThread, white, black).detach();
    }

    closesocket(listenFd);
    WSACleanup();
    return 0;
}
