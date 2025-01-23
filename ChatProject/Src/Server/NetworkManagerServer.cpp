#include "ServerPCH.h"
#include "NetworkManagerServer.h"

#include "Engine.h"
#include "PacketQueue.h"

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
	m_listenSocket.m_socket = Socket::CreateWSASocket();
	
	cout << "listenSocket ���� �Ϸ�" << endl;

	if (m_listenSocket.Bind("0.0.0.0", 50000) == SOCKET_ERROR) {
		cout << "bind error: " << WSAGetLastError() << endl;
		return;
	}

	cout << "bind �Ϸ�" << endl;

	if (listen(m_listenSocket.m_socket, 10) == SOCKET_ERROR) {
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
		reinterpret_cast<HANDLE>(m_listenSocket.m_socket),
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
		m_listenSocket.m_socket,			// ���� API�� �ʿ��� arg�� �� ������... ���� �� �ʿ����� �𸣰���.
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
	m_clientCandidateSocket.m_socket = Socket::CreateWSASocket();

	bool acceptExStatus = m_AcceptEx(
		m_listenSocket.m_socket,					// listenSocket
		m_clientCandidateSocket.m_socket,			// Accept�� �̷������ client socket���� ���Ѵ�.
		m_lpOutputBuf,								// ù ��° ������ ���, ���� �����ּ�, Ŭ�� ���� �ּ� ���� ����.
													// �� ������ �Ʒ� 3���� ����Ʈ �� ���� ������ GetAcceptExSockaddrs() �Լ��� ���ڷ�
													// ����/���� sockaddr ������ �Ľ��� ���� ���� �� �ִ�.
		0,											// ���ſ� ����� ������ ����Ʈ ��. 0�̸� �����ʹ� ���� �ʰ� accept�� �ϰڴٴ� �ǹ�.
		sizeof(sockaddr_in) + 16,					// ���� �ּ� ������ ���� ����� ����Ʈ ��. ���� ���������� �ִ� ���̺��� 16��ŭ Ŀ�� �Ѵ�.
		sizeof(sockaddr_in) + 16,					// ���� �ּ� ������ ���� ����� ����Ʈ ��. ���� ����.
		&m_dwBytes,									// ���� ����Ʈ ��. ���ʿ�.
		&m_listenSocket.m_readOverlappedStruct);	// lpOverlapped: ��û�� ó���ϴ� �� ���Ǵ� OVERLAPPED ����ü. NULL �Ұ�!
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
void NetworkManagerServer::ProcessIOCPEvent()
{
	GetCompletionStatus();

	// ���� �̺�Ʈ ������ ó���մϴ�.
	for (int i = 0; i < m_iocpEvent.m_eventCount; i++)
	{
		auto& readEvent = m_iocpEvent.m_events[i];
		ULONG_PTR completionKey = readEvent.lpCompletionKey;
		if (completionKey == 0) // listenSocket. AcceptEx�� ���� �ű� client ����
		{
			if (ProcessAcceptedClientSocketIOCP() == false)
				return;
		}
		else // clientSocket
		{
			shared_ptr<Socket> p_clientSocket = m_clientsMap[completionKey];

			// event�� WSASend�� �Ϸῡ ���� �߻��ߴٸ�, ��������.
			if (readEvent.lpOverlapped == &p_clientSocket->m_sendOverlappedStruct)
			{
				continue;
			}

			size_t sendBytes = readEvent.dwNumberOfBytesTransferred;
			
			// sendBytes == 0�� �� clientSocket ���� ���� �ʿ�
			if (sendBytes <= 0)
			{
				closesocket(p_clientSocket->m_socket);
				cout << "close socket: " << completionKey << endl;
				continue;
			}

			// ���� ���� ���
			p_clientSocket->m_receiveBuffer[sendBytes] = 0;

			ReceivePacketsIOCP(p_clientSocket);
		}
	}
}
bool NetworkManagerServer::GetCompletionStatus()
{
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
bool NetworkManagerServer::ProcessAcceptedClientSocketIOCP()
{
	cout << "listen Socket!!!" << endl;

	// m_clientCandidateSocket�κ��� �ű� client socket�� iocp�� �߰�
	// listenSocket�� ������ context�� clientSocket�� ����ȭ
	setsockopt(
		m_clientCandidateSocket.m_socket,
		SOL_SOCKET,
		SO_UPDATE_ACCEPT_CONTEXT,
		reinterpret_cast<const char*>(&m_listenSocket),
		sizeof(m_listenSocket));

	// SOCKET Ÿ���� UINT_PTR�� ���̴�. �Ʒ��� ���� ���� �����ϰ�
	// ���Ŀ� m_clientCandidateSocket�� �ٽ� listen�� ����ϴ���
	// m_clientCandidateSocket ������ ���ο� clientSocket�� �ּҸ� ������ ���� ���̴�.
	// // ac, 104, ... �̷� ������ ���.
	// ��·�� ����� ������ �ٸ� ���̴�.
	shared_ptr<Socket> clientSocket = make_shared<Socket>();
	clientSocket->m_socket = m_clientCandidateSocket.m_socket;

	// �ű� client�� IOCP�� �߰�
	const ULONG_PTR completionKey = reinterpret_cast<ULONG_PTR>(clientSocket.get());
	cout << "completionKey: " << completionKey << endl;

	if (AddSocketIOCP(clientSocket, completionKey) == nullptr) {
		cout << "Add IOCP error: " << WSAGetLastError() << endl;
		return false;
	}

	// ���� completionKey�� clientSocket ���� ���� map�� �����صд�.
	m_clientsMap.insert({ completionKey, clientSocket });

	// �ٽ� listenSocket�� accept�� ����
	// listenSocket.AcceptEx() ���·� ���� �� ���� �� ����. ���� �����͸� ����.
	m_clientCandidateSocket.m_socket = Socket::CreateWSASocket();
	AcceptEx();

	// ������ clientSocket�� recv�� ��ȯ
	Recv(clientSocket);

	return true;
}
HANDLE NetworkManagerServer::AddSocketIOCP(std::shared_ptr<Socket> clientSocket, const ULONG_PTR completionKey)
{
	return CreateIoCompletionPort(
		reinterpret_cast<HANDLE>(clientSocket->m_socket),
		mh_iocp,
		completionKey,
		0);
}
void NetworkManagerServer::ReceivePacketsIOCP(std::shared_ptr<Socket> p_clientSocket)
{
	auto& receiveQueue = PacketQueue::GetReceiveStaticInstance();
	receiveQueue.Push(p_clientSocket->m_receiveBuffer);

	//p_clientSocket->SetSendBuffer(
	//	p_clientSocket->m_receiveBuffer,
	//	sendBytes);

	//// Send ����
	//Send(*p_clientSocket, sendBytes);

	// �ٽ� ���� ���
	Recv(p_clientSocket);
}
int NetworkManagerServer::Recv(shared_ptr<Socket> clientSocket)
{
	WSABUF b;
	b.buf = clientSocket->m_receiveBuffer;
	b.len = clientSocket->MAX_RECEIVE_LENGTH;

	DWORD& numberOfBytesReceived = clientSocket->m_numberOfBytesReceived;
	
	// overlapped I/O�� ����Ǵ� ���� ���� ���� ä�����ϴ�.
	clientSocket->m_readFlags = 0;

	int retCode = WSARecv(
		clientSocket->m_socket,
		&b,										// lpBuffers.
		1,										// dwBufferCount. lpBuffers �迭�� ����ü ����.
		&numberOfBytesReceived,					// lpNumberOfBytesRecvd. TCP���� ��������������
		&clientSocket->m_readFlags,
		&clientSocket->m_readOverlappedStruct,
		NULL);									// lpCompletionRoutine. ���� �۾� �Ϸ� ��ƾ�� ���� ������.

	return retCode;
}
void NetworkManagerServer::SendPacketsIOCP()
{
	auto& sendQueue = PacketQueue::GetSendStaticInstance();

	while (sendQueue.Empty() == false)
	{
		string data = sendQueue.Front();
		
		// broadcast
		for (auto clientSocketPair : m_clientsMap)
		{
			auto clientSocket = clientSocketPair.second;
			clientSocket->SetSendBuffer(data.c_str(), data.size());
			Send(clientSocket, data.size());
		}
	}
}
int NetworkManagerServer::Send(shared_ptr<Socket> clientSocket, size_t len)
{
	WSABUF b;
	b.buf = clientSocket->m_sendBuffer;
	b.len = static_cast<ULONG>(len);

	int retCode = WSASend(
		clientSocket->m_socket,
		&b,
		1,
		&clientSocket->m_numberOfBytesSent,
		clientSocket->m_sendFlags,
		&clientSocket->m_sendOverlappedStruct,
		NULL);

	return retCode;
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
