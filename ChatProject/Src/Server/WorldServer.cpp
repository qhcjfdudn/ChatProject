#include "ServerPCH.h"
#include "WorldServer.h"

#include "PacketQueue.h"
#include "Shuttlecock.h"
#include "LinkingContext.h"
#include "Constant.h"

WorldServer& WorldServer::GetInstance() {
	static WorldServer instance;
	return instance;
}

void WorldServer::InitWorld()
{
	// Shuttlecock �����
	Vector2 position(0, 0);
	Vector2 velocity(0, 1);
	auto shuttlecock = make_shared<Shuttlecock>(position, velocity);
	
	_gameObjects.push_back(shuttlecock);

	// LinkingContext�� ���
	auto& linkingContext = LinkingContext::GetInstance();
	linkingContext.AddGameObject(shuttlecock);

	_lastFixedUpdateTime = system_clock::now();
}

void WorldServer::FixedUpdate() {
	// �Ʒ� receive packet�� �ٷ�� �ڵ�� ���� ReplicationManager�� �Բ�
	// NetworkManager���� �����ϵ��� ����
	auto& receiveQueue = PacketQueue::GetReceiveStaticInstance();
	auto& sendQueue = PacketQueue::GetSendStaticInstance();

	while (receiveQueue.Empty() == false)
	{
		std::string received = receiveQueue.Front();

		const local_time<system_clock::duration> now = zoned_time{ current_zone(), system_clock::now() }.get_local_time();
		cout << "[" << now << "] received: " << received << endl;

		sendQueue.Push(received);
	}

	system_clock::time_point currentTime = system_clock::now();
	std::chrono::duration<double> elapsedTime = currentTime - _lastFixedUpdateTime;
	if (elapsedTime.count() >= Constant::FIXED_TIMESTEP)
	{
		_lastFixedUpdateTime = currentTime;
		
		const local_time<system_clock::duration> now = zoned_time{ current_zone(), system_clock::now() }.get_local_time();
		cout << "[" << now << "] FixedUpdate" << endl;

		for (auto& gameObject : _gameObjects)
		{
			gameObject->FixedUpdate();
			// replication update code here (send packet)
		}

	}
}

