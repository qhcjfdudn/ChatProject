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

			std::cout << "연결 완료" << endl;
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

		// 임시 종료 조건
		if (strcmp(packet, "quit") == 0)
			engine.isRunning = false;
	}

	if (m_clientSocketItersToErase.size() > 0) {
		closeSockets();
	}
}
void NetworkManagerServer::SendPackets() {
	// World의 변경사항으로 client에게 보낼 내용이 있을 때 전송
	// eventQueue 구현 필요
	
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
	// Overlapped IO 위한 listen socket 생성
	m_listenSocket = WSASocket(
		AF_INET, 
		SOCK_STREAM, 
		0, 
		nullptr, 
		0, 
		WSA_FLAG_OVERLAPPED);
	
	cout << "listenSocket 생성 완료" << endl;
		 
	sockaddr_in s_in = {};
	s_in.sin_family = AF_INET;
	s_in.sin_addr.S_un.S_addr = INADDR_ANY;
	s_in.sin_port = htons(50000);

	if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&s_in), sizeof(s_in)) == SOCKET_ERROR) {
		cout << "bind error: " << WSAGetLastError() << endl;
		return;
	}

	cout << "bind 완료" << endl;

	if (listen(m_listenSocket, 10) == SOCKET_ERROR) {
		cout << "listen error: " << WSAGetLastError() << endl;
		return;
	}

	cout << "listen 완료" << endl;

	// IOCP 생성
	mh_iocp = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE, 
		nullptr, 
		reinterpret_cast<ULONG_PTR>(nullptr), 
		m_threadCount);

	cout << "IOCP 생성 완료" << endl;

	// IOCP에 listen socket 추가
	if (CreateIoCompletionPort(
		reinterpret_cast<HANDLE>(m_listenSocket),
		mh_iocp, 
		reinterpret_cast<ULONG_PTR>(nullptr), 
		0) == nullptr) {
		cout << "Add IOCP error: " << GetLastError() << endl;
		//cout << "Add IOCP error: " << WSAGetLastError() << endl;
		return;
	}

	cout << "IOCP에 listenSocket 등록 완료" << endl;
	
	// WSAIoctl로부터 AcceptEx 함수 포인터 할당
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;

	if (WSAIoctl(
		m_listenSocket,						// 소켓 API라 필요한 arg인 것 같은데... 아직 왜 필요한지 모르겠음.
		SIO_GET_EXTENSION_FUNCTION_POINTER,	// AcceptEx 함수 포인터를 얻기 위한 제어 코드
		&guidAcceptEx,						// 얻고자 하는 함수 이름의 지정된 값(WSAID_ACCEPTEX) 사용.
		sizeof(guidAcceptEx),
		&m_AcceptEx,						// 요청에 대한 출력 버퍼. AcceptEx 함수 포인터 출력.
		sizeof(m_AcceptEx),
		&dwBytes,							// 출력버퍼로 출력된 개수. <- 애매
		nullptr,							// lpOverlapped: WSAOVERLAPPED 구조체 포인터. 지금 불필요.
		nullptr) == SOCKET_ERROR) {			// lpCompletionRoutine: 작업 완료 후 호출할 루틴 전달 가능. // 지금 불필요.

		cout << "WSAIoctl error: " << WSAGetLastError() << endl;
		}

	if (m_AcceptEx == nullptr) {
		cout << "Getting AcceptEx ptr failed." << endl;

		return;
	}

	cout << "WSAIoctl 반환 및 m_AcceptEx 함수 포인터 획득 완료" << endl;

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
		m_clientCandidateSocket,	// Accept가 이루어지면 client socket으로 변한다.
		m_lpOutputBuf,				// 첫 번째 데이터 블록, 서버 로컬주소, 클라 원격 주소 수신 버퍼
		0,							// 수신에 사용할 데이터 바이트 수. 0이면 데이터는 받지 않고 accept만 하겠다는 의미.
		sizeof(sockaddr_in) + 16,	// 로컬 주소 정보를 위해 예약된 바이트 수. 전송 프로토콜의 최대 길이보다 16만큼 커야 한다.
		sizeof(sockaddr_in) + 16,	// 원격 주소 정보를 위해 예약된 바이트 수. 위와 동일.
		&m_dwBytes,					// 받은 바이트 수. 불필요.
		&m_readOverlappedStruct);	// lpOverlapped: 요청을 처리하는 데 사용되는 OVERLAPPED 구조체. NULL 불가!
	// 에러가 없다면 ret은 TRUE이다.

	if (acceptExStatus == false)
	{
		int errorCode = WSAGetLastError();

		if (errorCode == ERROR_IO_PENDING) // 아직 IO 처리 중. 정상 상태
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
		cout << "m_AcceptEx 함수 수행 완료" << endl;
	}
}

void NetworkManagerServer::ReceivePacketsIOCP()
{
	GetCompletionStatus();

	// 받은 이벤트 각각을 처리합니다.
	for (int i = 0; i < m_iocpEvent.m_eventCount; i++)
	{
		auto& readEvent = m_iocpEvent.m_events[i];
		if (readEvent.lpCompletionKey == 0) // listenSocket
		{
			cout << "listen Socket!!!" << endl;

			// m_clientCandidateSocket로부터 신규 client socket을 iocp에 추가
			


			// 다시 listenSocket을 accept
			AcceptEx();
		}
	}
}

bool NetworkManagerServer::GetCompletionStatus()
{
	//cout << "GetCompletionStatus() 호출" << endl;

	bool ret = GetQueuedCompletionStatusEx(
		mh_iocp,							// IOCP 객체
		m_iocpEvent.m_events,					// 처리가 완료된 event를 수신하는 배열
		MAX_IOCP_EVENT_COUNT,				// 최대 event 개수
		(ULONG*)&m_iocpEvent.m_eventCount,	// 수신한 event 개수를 받을 변수
		m_timeoutMs,							// 다시 분석 필요
		FALSE);								// fAlertable: ?????

	if (ret == false)	// 실패 시
	{
		int errorCode = WSAGetLastError();
		
		if (errorCode == WSA_WAIT_TIMEOUT)	// timeoutMs 동안 event가 발생하지 않았다.
											// 별도 처리할 내용 없음
		{}
		else
		{
			cout << "GetQueuedCompletionStatusEx 실패" << endl;
			cout << errorCode << endl;
		}
		
		m_iocpEvent.m_eventCount = 0;			// 실패 시 수동으로 변경 필요 코드
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
	m_acceptSocketThread = nullptr;
	
	mh_iocp = nullptr;
	m_threadCount = 1;
	
	m_AcceptEx = nullptr;
	m_listenSocket = {};
	m_clientCandidateSocket = {};
	memset(&m_lpOutputBuf, 0, sizeof(m_lpOutputBuf));
	m_dwBytes = {};

	m_readOverlappedStruct = {};
	
	memset(&m_iocpEvent, 0, sizeof(m_iocpEvent));
	m_timeoutMs = 100;


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
