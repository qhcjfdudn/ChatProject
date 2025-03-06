#include "ServerPCH.h"
#include "WorldServer.h"

#include "PacketQueue.h"
#include "OutputMemoryBitStream.h"
#include "ReplicationManager.h"
#include "LinkingContext.h"

#include "Constant.h"

#include "Shuttlecock.h"

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

	_lastFixedUpdateTime = _lastPacketUpdateTime = system_clock::now();
}

void WorldServer::Update()
{
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
}

void WorldServer::FixedUpdate() {
	system_clock::time_point currentTime = system_clock::now();
	std::chrono::duration<double> elapsedTime = currentTime - _lastFixedUpdateTime;

	if (elapsedTime.count() < Constant::FIXED_UPDATE_TIMESTEP)
		return;

	const local_time<system_clock::duration> now = zoned_time{ current_zone(), currentTime }.get_local_time();
	cout << "[" << now << "] FixedUpdate" << endl;

	for (auto& gameObject : _gameObjects)
	{
		NetworkId_t networkId = gameObject->GetNetworkId();
		if (gameObject->FixedUpdate() && 
			_updatedObjectNetworkIds.find(networkId) == _updatedObjectNetworkIds.end())
		{
			_updatedObjectNetworkIds.insert(networkId);
			_pendingSerializationQueue.push(networkId);
		}
	}

	_lastFixedUpdateTime = currentTime;
}

void WorldServer::WriteWorldStateToStream()
{
	system_clock::time_point currentTime = system_clock::now();
	std::chrono::duration<double> elapsedTime = currentTime - _lastPacketUpdateTime;

	if (elapsedTime.count() < Constant::PACKET_PERIOD)
		return;

	_lastPacketUpdateTime = currentTime;

	const local_time<system_clock::duration> now = zoned_time{ current_zone(), currentTime }.get_local_time();
	cout << "[" << now << "] WriteWorldStateToStream" << endl;

	auto& linkingContext = LinkingContext::GetInstance();
	auto& replicationManager = ReplicationManager::GetInstance();
	OutputMemoryBitStream inStream;

	// delta�� �ִ� ��ü�� Update �ϰ� �ʹ�.
	// => Update�� �ִ� ��ü�� GUID�� ����� Queue�� ����
	// ť�� ���Ҹ� ��� pop �ϸ鼭 stream�� ���
	// PacketQueue�� ���� �� stream ���� ����Ǳ� ������, ���⼭ stream�� �����ϰ� �Ҹ���ѵ�
	// ���� ����.
	int pending_serialization_count = static_cast<int>(_pendingSerializationQueue.size());
	while (pending_serialization_count--)
	{
		NetworkId_t networkId = _pendingSerializationQueue.front();
		_pendingSerializationQueue.pop();

		auto gameObject = linkingContext.GetGameObject(networkId);
		replicationManager.ReplicateUpdate(inStream, gameObject);

		_updatedObjectNetworkIds.erase(networkId);
	}

	if (inStream.GetBitLength() <= 0)
		return;

	auto& sendQueue = PacketQueue::GetSendStaticInstance();
	sendQueue.Push(inStream.GetBufferPtr());
}

