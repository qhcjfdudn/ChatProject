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

void NetworkManagerServer::InitIOCP() {
	// Overlapped IO ���� listen socket ����
	m_listenSocket = WSASocket(
		AF_INET, 
		SOCK_STREAM, 
		0, 
		nullptr, 
		0, 
		WSA_FLAG_OVERLAPPED);
	
	cout << "listenSocket ���� �Ϸ�" << endl;
		 
	sockaddr_in s_in = {};
	s_in.sin_family = AF_INET;
	s_in.sin_addr.S_un.S_addr = INADDR_ANY;
	s_in.sin_port = htons(50000);

	if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&s_in), sizeof(s_in)) == SOCKET_ERROR) {
		cout << "bind error: " << WSAGetLastError() << endl;
		return;
	}

	cout << "bind �Ϸ�" << endl;

	if (listen(m_listenSocket, 10) == SOCKET_ERROR) {
		cout << "listen error: " << WSAGetLastError() << endl;
		return;
	}

	cout << "listen �Ϸ�" << endl;

	// IOCP ����
	mh_iocp = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE, 
		nullptr, 
		reinterpret_cast<ULONG_PTR>(nullptr), 
		m_threadCount);

	cout << "IOCP ���� �Ϸ�" << endl;

	// IOCP�� listen socket �߰�
	if (CreateIoCompletionPort(
		reinterpret_cast<HANDLE>(m_listenSocket),
		mh_iocp, 
		reinterpret_cast<ULONG_PTR>(nullptr), 
		0) == nullptr) {
		cout << "Add IOCP error: " << GetLastError() << endl;
		//cout << "Add IOCP error: " << WSAGetLastError() << endl;
		return;
	}

	cout << "IOCP�� listenSocket ��� �Ϸ�" << endl;
	
	// WSAIoctl�κ��� AcceptEx �Լ� ������ �Ҵ�
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;

	if (WSAIoctl(
		m_listenSocket,						// ���� API�� �ʿ��� arg�� �� ������... ���� �� �ʿ����� �𸣰���.
		SIO_GET_EXTENSION_FUNCTION_POINTER,	// AcceptEx �Լ� �����͸� ��� ���� ���� �ڵ�
		&guidAcceptEx,						// ����� �ϴ� �Լ� �̸��� ������ ��(WSAID_ACCEPTEX) ���.
		sizeof(guidAcceptEx),
		&m_AcceptEx,						// ��û�� ���� ��� ����. AcceptEx �Լ� ������ ���.
		sizeof(m_AcceptEx),
		&dwBytes,							// ��¹��۷� ��µ� ����. <- �ָ�
		nullptr,							// lpOverlapped: WSAOVERLAPPED ����ü ������. ���� ���ʿ�.
		nullptr) == SOCKET_ERROR) {			// lpCompletionRoutine: �۾� �Ϸ� �� ȣ���� ��ƾ ���� ����. // ���� ���ʿ�.

		cout << "WSAIoctl error: " << WSAGetLastError() << endl;
		}

	if (m_AcceptEx == nullptr) {
		cout << "Getting AcceptEx ptr failed." << endl;

		return;
	}

	cout << "WSAIoctl ��ȯ �� m_AcceptEx �Լ� ������ ȹ�� �Ϸ�" << endl;

	AcceptEx();
}

void NetworkManagerServer::AcceptEx()
{
	m_clientCandidateSocket = WSASocket(
		AF_INET,
		SOCK_STREAM,
		0,
		nullptr,
		0,
		WSA_FLAG_OVERLAPPED);

	bool acceptExStatus = m_AcceptEx(
		m_listenSocket,				// listenSocket
		m_clientCandidateSocket,	// Accept�� �̷������ client socket���� ���Ѵ�.
		m_lpOutputBuf,				// ù ��° ������ ���, ���� �����ּ�, Ŭ�� ���� �ּ� ���� ����
		0,							// ���ſ� ����� ������ ����Ʈ ��. 0�̸� �����ʹ� ���� �ʰ� accept�� �ϰڴٴ� �ǹ�.
		sizeof(sockaddr_in) + 16,	// ���� �ּ� ������ ���� ����� ����Ʈ ��. ���� ���������� �ִ� ���̺��� 16��ŭ Ŀ�� �Ѵ�.
		sizeof(sockaddr_in) + 16,	// ���� �ּ� ������ ���� ����� ����Ʈ ��. ���� ����.
		&m_dwBytes,					// ���� ����Ʈ ��. ���ʿ�.
		&m_readOverlappedStruct);	// lpOverlapped: ��û�� ó���ϴ� �� ���Ǵ� OVERLAPPED ����ü. NULL �Ұ�!
	// ������ ���ٸ� ret�� TRUE�̴�.

	if (acceptExStatus == false)
	{
		int errorCode = WSAGetLastError();

		if (errorCode == ERROR_IO_PENDING) // ���� IO ó�� ��. ���� ����
		{
		}
		else
		{
			cout << "AcceptEx() Error" << endl;
			cout << errorCode << endl;
		}
	}
	else
	{
		cout << "m_AcceptEx �Լ� ���� �Ϸ�" << endl;
	}
}

void NetworkManagerServer::ReceivePacketsIOCP()
{
	GetCompletionStatus();

	// ���� �̺�Ʈ ������ ó���մϴ�.
	for (int i = 0; i < m_iocpEvent.m_eventCount; i++)
	{
		auto& readEvent = m_iocpEvent.m_events[i];
		if (readEvent.lpCompletionKey == 0) // listenSocket
		{
			cout << "listen Socket!!!" << endl;

			// m_clientCandidateSocket�κ��� �ű� client socket�� iocp�� �߰�
			


			// �ٽ� listenSocket�� accept
			AcceptEx();
		}
	}
}

bool NetworkManagerServer::GetCompletionStatus()
{
	//cout << "GetCompletionStatus() ȣ��" << endl;

	bool ret = GetQueuedCompletionStatusEx(
		mh_iocp,							// IOCP ��ü
		m_iocpEvent.m_events,					// ó���� �Ϸ�� event�� �����ϴ� �迭
		MAX_IOCP_EVENT_COUNT,				// �ִ� event ����
		(ULONG*)&m_iocpEvent.m_eventCount,	// ������ event ������ ���� ����
		m_timeoutMs,							// �ٽ� �м� �ʿ�
		FALSE);								// fAlertable: ?????

	if (ret == false)	// ���� ��
	{
		int errorCode = WSAGetLastError();
		
		if (errorCode == WSA_WAIT_TIMEOUT)	// timeoutMs ���� event�� �߻����� �ʾҴ�.
											// ���� ó���� ���� ����
		{}
		else
		{
			cout << "GetQueuedCompletionStatusEx ����" << endl;
			cout << errorCode << endl;
		}
		
		m_iocpEvent.m_eventCount = 0;			// ���� �� �������� ���� �ʿ� �ڵ�
	}

	return ret;
}

void NetworkManagerServer::closeSockets()
{
	for (auto clientSocketIter : m_clientSocketItersToErase)
	{
		if (closesocket(*clientSocketIter) == SOCKET_ERROR)
		{
			cout << "closesocket error: " << WSAGetLastError() << endl;
		}

		m_clientSockets.erase(clientSocketIter);
	}

	m_clientSocketItersToErase.clear();
}

NetworkManagerServer::NetworkManagerServer()
{
	if (WSAStartup(MAKEWORD(2, 2), &m_wsa) != 0)
	{
		cout << "WSAStartup failed" << endl;
		
		return;
	}
	cout << "WSAStartup" << endl;
	cout << m_clientSockets.size() << endl;
}
NetworkManagerServer::~NetworkManagerServer() {
	for (const auto& clientSocket : m_clientSockets)
	{
		if (closesocket(clientSocket) == SOCKET_ERROR)
		{
			cout << "closesocket error: " << WSAGetLastError() << endl;
		}
	}

	WSACleanup();
	cout << "WSACleanup" << endl;

	m_acceptSocketThread->join();
}
