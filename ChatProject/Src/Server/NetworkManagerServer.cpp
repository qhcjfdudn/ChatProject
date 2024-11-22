#include "ServerPCH.h"
#include "NetworkManagerServer.h"

#include "Engine.h"

NetworkManagerServer& NetworkManagerServer::GetInstance() {
	static NetworkManagerServer sInstance;
	return sInstance;
}
void NetworkManagerServer::Init() {
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in s_in = {};
	s_in.sin_family = AF_INET;
	s_in.sin_addr.S_un.S_addr = INADDR_ANY;
	s_in.sin_port = htons(50000);
	if (bind(s, reinterpret_cast<sockaddr*>(&s_in), sizeof(s_in)) == SOCKET_ERROR) {
		cout << "bind error: " << WSAGetLastError() << endl;
		return;
	}

	if (listen(s, 10) == SOCKET_ERROR) {
		cout << "listen error: " << WSAGetLastError() << endl;
		return;
	}

	m_acceptSocketThread = new std::thread([s] {
		SOCKET client_socket = {};
		sockaddr_in client = {};
		int client_size = sizeof(client);

		Engine& engine = Engine::GetInstance();
		auto& nmsInstance = NetworkManagerServer::GetInstance();
		while (engine.isRunning) {
			cout << "accepting" << endl;
			if ((client_socket = accept(s,
				reinterpret_cast<sockaddr*>(&client),
				&client_size)) == SOCKET_ERROR) {
				cout << "accept error: " << WSAGetLastError() << endl;

				continue;
			}

			std::cout << "���� �Ϸ�" << endl;
			std::cout << "Client IP: " << inet_ntoa(client.sin_addr) << endl;
			std::cout << "Port: " << ntohs(client.sin_port) << endl;

			nmsInstance.m_clientSockets.push_back(client_socket);
		}

		cout << "acceptSocketThread end" << endl;
		});
}

void NetworkManagerServer::InitIOCP() {
	SOCKET s = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	
	sockaddr_in s_in = {};
	s_in.sin_family = AF_INET;
	s_in.sin_addr.S_un.S_addr = INADDR_ANY;
	s_in.sin_port = htons(50000);

	if (bind(s, reinterpret_cast<sockaddr*>(&s_in), sizeof(s_in)) == SOCKET_ERROR) {
		cout << "bind error: " << WSAGetLastError() << endl;
		return;
	}

	if (listen(s, 10) == SOCKET_ERROR) {
		cout << "listen error: " << WSAGetLastError() << endl;
		return;
	}

	mh_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, m_threadCount);



}

void NetworkManagerServer::ReceivePackets() {
	char packet[1000];
	auto& engine = Engine::GetInstance();
	
	for (auto it = m_clientSockets.begin(); it != m_clientSockets.end(); it++) {
		memset(packet, 0, sizeof(packet));
		if (recv(*it, packet, sizeof(packet), 0) == SOCKET_ERROR) {
			cout << "recv error: " << WSAGetLastError() << endl;
			if (WSAGetLastError() == 10054) {
				m_clientSocketItersToErase.push_back(it);
			}
			continue;
		}
		cout << packet << endl;

		// �ӽ� ���� ����
		if (strcmp(packet, "quit") == 0)
			engine.isRunning = false;
	}

	if (m_clientSocketItersToErase.size() > 0) {
		closeSockets();
	}
}
void NetworkManagerServer::SendPackets() {
	// World�� ����������� client���� ���� ������ ���� �� ����
	// eventQueue ���� �ʿ�
	
	//char str[20];
	//for (const auto& clientSocket : clientSockets) {
	//	if (send(clientSocket, str, static_cast<int>(strlen(str)), 0) == SOCKET_ERROR) {
	//		cout << "send error: " << WSAGetLastError() << endl;
	//	}
	//}

	if (m_clientSocketItersToErase.size() > 0) {
		closeSockets();
	}
}

void NetworkManagerServer::closeSockets() {
	for (auto clientSocketIter : m_clientSocketItersToErase) {
		if (closesocket(*clientSocketIter) == SOCKET_ERROR) {
			cout << "closesocket error: " << WSAGetLastError() << endl;
		}

		m_clientSockets.erase(clientSocketIter);
	}

	m_clientSocketItersToErase.clear();
}

NetworkManagerServer::NetworkManagerServer()
	: m_acceptSocketThread(nullptr), mh_iocp(nullptr), m_threadCount(1) {
	if (WSAStartup(MAKEWORD(2, 2), &m_wsa) != 0) {
		cout << "WSAStartup failed" << endl;
		
		return;
	}
	cout << "WSAStartup" << endl;
	cout << m_clientSockets.size() << endl;
}
NetworkManagerServer::~NetworkManagerServer() {
	for (const auto& clientSocket : m_clientSockets) {
		if (closesocket(clientSocket) == SOCKET_ERROR) {
			cout << "closesocket error: " << WSAGetLastError() << endl;
		}
	}

	WSACleanup();
	cout << "WSACleanup" << endl;

	m_acceptSocketThread->join();
}
