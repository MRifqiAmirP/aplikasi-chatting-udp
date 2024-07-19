#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <set>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

// Fungsi perbandingan untuk sockaddr_in
struct sockaddr_in_compare {
    bool operator()(const sockaddr_in& a, const sockaddr_in& b) const {
        if (a.sin_addr.s_addr < b.sin_addr.s_addr) {
            return true;
        }
        if (a.sin_addr.s_addr == b.sin_addr.s_addr) {
            return a.sin_port < b.sin_port;
        }
        return false;
    }
};

mutex clients_mutex;
set<sockaddr_in, sockaddr_in_compare> clients;

void broadcast_message(SOCKET server_socket, const char* message, int message_len, const sockaddr_in& sender_addr) {
    lock_guard<mutex> lock(clients_mutex);
    for (const auto& client_addr : clients) {
        // Jangan broadcast ke pengirim
        if (client_addr.sin_addr.s_addr == sender_addr.sin_addr.s_addr &&
            client_addr.sin_port == sender_addr.sin_port) {
            continue;
        }
        int addr_len = sizeof(client_addr);
        int bytes_sent = sendto(server_socket, message, message_len, 0, (struct sockaddr*)&client_addr, addr_len);
        if (bytes_sent == SOCKET_ERROR) {
            cerr << "Failed to send message to client. Error: " << WSAGetLastError() << endl;
        }
    }
}


int main() {
    WSADATA wsaData;
    SOCKET server_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed. Error: " << WSAGetLastError() << endl;
        return EXIT_FAILURE;
    }

    // Create a socket for the server
    server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed. Error: " << WSAGetLastError() << endl;
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Set up the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9999);

    // Bind the socket to the address and port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Bind failed. Error: " << WSAGetLastError() << endl;
        closesocket(server_socket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    cout << "Server is running and waiting for incoming messages..." << endl;

    while (true) {
        char buffer[1024];
        sockaddr_in sender_addr;
        int sender_addr_size = sizeof(sender_addr);
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recvfrom(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &addr_len);
        if (bytes_received == SOCKET_ERROR) {
            cerr << "recvfrom failed. Error: " << WSAGetLastError() << endl;
            continue;
        }

        cout << "Received message: " << buffer << endl;

        // Add the client to the set of known clients
        {
            lock_guard<mutex> lock(clients_mutex);
            clients.insert(client_addr);
        }

        // Broadcast the received message to all clients
        broadcast_message(server_socket, buffer, bytes_received, sender_addr);
    }

    // Clean up and close the socket
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
