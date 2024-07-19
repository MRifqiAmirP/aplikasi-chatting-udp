#pragma once

#include <WinSock2.h>
#include <msclr/marshal_cppstd.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9999
#define BUFFER_SIZE 1024

int err;

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;

namespace T2UDPCLIENT062230701416 {

    public ref class MyForm : public System::Windows::Forms::Form
    {
    private:
        // Other members...

        // Use pin_ptr for managing unmanaged struct within a managed class
        sockaddr_in* server_addr;
    public:
        MyForm(void)
        {
            InitializeComponent();

            server_addr = new sockaddr_in();
            memset(server_addr, 0, sizeof(sockaddr_in));
        }

    protected:
        ~MyForm()
        {
            delete server_addr;
            server_addr = nullptr;

            if (components)
            {
                delete components;
            }
            if (client_socket != INVALID_SOCKET)
            {
                closesocket(client_socket);
                WSACleanup();
            }
        }

    private: System::Windows::Forms::ListBox^ listBox1;
    private: System::Windows::Forms::TextBox^ textBox1;
    private: System::Windows::Forms::TextBox^ textBoxNickname;
    private: System::Windows::Forms::Button^ button1;
    private: System::Windows::Forms::Button^ buttonConnect;

    private:
        System::ComponentModel::Container^ components;
        SOCKET client_socket;

#pragma region Windows Form Designer generated code
        void InitializeComponent(void)
        {
            this->listBox1 = (gcnew System::Windows::Forms::ListBox());
            this->textBox1 = (gcnew System::Windows::Forms::TextBox());
            this->textBoxNickname = (gcnew System::Windows::Forms::TextBox());
            this->button1 = (gcnew System::Windows::Forms::Button());
            this->buttonConnect = (gcnew System::Windows::Forms::Button());
            this->SuspendLayout();
            // 
            // listBox1
            // 
            this->listBox1->FormattingEnabled = true;
            this->listBox1->ItemHeight = 16;
            this->listBox1->Location = System::Drawing::Point(12, 12);
            this->listBox1->Name = L"listBox1";
            this->listBox1->Size = System::Drawing::Size(431, 276);
            this->listBox1->TabIndex = 0;
            // 
            // textBox1
            // 
            this->textBox1->Location = System::Drawing::Point(12, 310);
            this->textBox1->Name = L"textBox1";
            this->textBox1->Size = System::Drawing::Size(289, 22);
            this->textBox1->TabIndex = 1;
            // 
            // textBoxNickname
            // 
            this->textBoxNickname->Location = System::Drawing::Point(12, 372);
            this->textBoxNickname->Name = L"textBoxNickname";
            this->textBoxNickname->Size = System::Drawing::Size(289, 22);
            this->textBoxNickname->TabIndex = 3;
            // 
            // button1
            // 
            this->button1->Location = System::Drawing::Point(327, 310);
            this->button1->Name = L"button1";
            this->button1->Size = System::Drawing::Size(116, 23);
            this->button1->TabIndex = 2;
            this->button1->Text = L"Kirim";
            this->button1->UseVisualStyleBackColor = true;
            this->button1->Click += gcnew System::EventHandler(this, &MyForm::button1_Click);
            // 
            // buttonConnect
            // 
            this->buttonConnect->Location = System::Drawing::Point(327, 371);
            this->buttonConnect->Name = L"buttonConnect";
            this->buttonConnect->Size = System::Drawing::Size(116, 23);
            this->buttonConnect->TabIndex = 4;
            this->buttonConnect->Text = L"Connect";
            this->buttonConnect->UseVisualStyleBackColor = true;
            this->buttonConnect->Click += gcnew System::EventHandler(this, &MyForm::buttonConnect_Click);
            // 
            // MyForm
            // 
            this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(470, 450);
            this->Controls->Add(this->buttonConnect);
            this->Controls->Add(this->textBoxNickname);
            this->Controls->Add(this->button1);
            this->Controls->Add(this->textBox1);
            this->Controls->Add(this->listBox1);
            this->Name = L"MyForm";
            this->Text = L"MyForm";
            this->ResumeLayout(false);
            this->PerformLayout();

        }
#pragma endregion

    private:
        void InitializeSocket()
        {
            WSADATA ws;
            struct sockaddr_in server_address;

            // Initialize Winsock
            if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
                MessageBox::Show("Could not initialize the Socket API! Error: " + WSAGetLastError());
                Application::Exit();
            }

            // Create socket
            client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (client_socket == INVALID_SOCKET) {
                MessageBox::Show("Failed to open socket! Error: " + WSAGetLastError());
                WSACleanup();
                Application::Exit();
            }

            // Setup server address
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(SERVER_PORT); // server_port should be defined
            // Convert server IP address from text to binary form
            if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) { // server_ip should be defined
                MessageBox::Show("Invalid server IP address");
                WSACleanup();
                Application::Exit();
                return;
            }

            // Send nickname to server
            String^ nickname = textBoxNickname->Text;
            char* nick = (char*)(void*)Marshal::StringToHGlobalAnsi(nickname).ToPointer();
            int err = sendto(client_socket, nick, strlen(nick), 0, (struct sockaddr*)&server_address, sizeof(server_address));
            if (err < 0) {
                MessageBox::Show("Failed to send! Error: " + WSAGetLastError());
                WSACleanup();
                Application::Exit();
            }
            Marshal::FreeHGlobal(IntPtr(nick));


            // Start receiving messages in a separate thread
            ThreadStart^ threadDelegate = gcnew ThreadStart(this, &MyForm::ReceiveMessages);
            Thread^ recvThread = gcnew Thread(threadDelegate);
            recvThread->Start();
        }

        void ReceiveMessages()
        {
            char buffer[BUFFER_SIZE];
            int recv_size;
            int server_addr_size = sizeof(sockaddr_in);
            while (true) {
                memset(buffer, 0, BUFFER_SIZE);
                recv_size = recvfrom(client_socket, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);
                if (recv_size <= 0) {
                    MessageBox::Show("Server disconnected. Error: " + WSAGetLastError());
                    // If connection with server is lost, close socket and clean up resources
                    closesocket(client_socket);
                    WSACleanup();
                    Application::Exit();
                }
                buffer[recv_size] = '\0';
                String^ message = gcnew String(buffer);
                this->Invoke(gcnew Action<String^>(this, &MyForm::AddMessage), message);
            }
        }

        void AddMessage(String^ message)
        {
            listBox1->Items->Add(message);
            listBox1->TopIndex = listBox1->Items->Count - 1; // Auto-scroll to the bottom
        }

        void button1_Click(System::Object^ sender, System::EventArgs^ e)
        {
            String^ message = textBox1->Text;
            char* msg = (char*)(void*)Marshal::StringToHGlobalAnsi(message).ToPointer();

            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = inet_addr(SERVER_IP); // Ensure SERVER_IP is valid
            server_addr.sin_port = htons(SERVER_PORT); // Ensure SERVER_PORT is correct

            int server_addr_size = sizeof(server_addr);
            int sendResult = sendto(client_socket, msg, strlen(msg), 0, (struct sockaddr*)&server_addr, server_addr_size);

            if (sendResult == SOCKET_ERROR) {
                MessageBox::Show("Failed to send message. Error: " + WSAGetLastError());
            }
            else {
                textBox1->Clear();
            }

            Marshal::FreeHGlobal(IntPtr(msg));
        }



        void buttonConnect_Click(System::Object^ sender, System::EventArgs^ e)
        {
            InitializeSocket();
        }
    };
}
