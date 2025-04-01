#include "ServerPCH.h"
#include "WorldServer.h"

#include "Engine.h"
#include "PacketQueue.h"
#include "OutputMemoryBitStream.h"
#include "ReplicationManager.h"
#include "LinkingContext.h"

#include "Constant.h"

#include "Shuttlecock.h"

#include "GetRequiredBits.h"

WorldServer& WorldServer::GetInstance() {
	static WorldServer instance;
	return instance;
}

void WorldServer::InitLevel()
{
	// engine���κ��� �ʱ� object ����
	auto& engineInstance = Engine::GetInstance();

	engineInstance.CreatePlain(0.f, 1.f, 0.f, 0.f);
	
	// Net
	engineInstance.CreateBox2DStatic(PxVec2{ 0, 2.5 }, 0.5, 2.5);

	// Shuttlecock
	PxVec2 location(-3, 10);
	PxVec2 velocity(2, 5);
	auto shuttlecock = make_shared<Shuttlecock>(location, velocity);

	_gameObjects.push_back(shuttlecock);

	// LinkingContext�� ���
	_linkingContext.AddGameObject(shuttlecock);
}

void WorldServer::RemoveAll()
{
	auto& linkingContext = LinkingContext::GetInstance();

	for (auto& gameObject : _gameObjects)
		linkingContext.RemoveGameObject(gameObject);

	_gameObjects.clear();
}

void WorldServer::Update()
{
	// �Ʒ� receive packet�� �ٷ�� �ڵ�� ���� ReplicationManager�� �Բ�
	// NetworkManager���� �����ϵ��� ����
	auto& receiveQueue = PacketQueue::GetReceiveStaticInstance();
	auto& sendQueue = PacketQueue::GetSendStaticInstance();

	while (receiveQueue.Empty() == false)
	{
		shared_ptr<Packet> received = receiveQueue.Front();

		const local_time<system_clock::duration> now = zoned_time{ current_zone(), system_clock::now() }.get_local_time();
		cout << "[" << now << "] received: " << received->GetBuffer() << endl;

		sendQueue.PushCopy(*received);
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
		NetworkId_t networkId = _linkingContext.GetNetworkId(gameObject);

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
	
	if (_pendingSerializationQueue.empty())
		return;

	const local_time<system_clock::duration> now = zoned_time{ current_zone(), currentTime }.get_local_time();
	cout << "[" << now << "] WriteWorldStateToStream" << endl;
	cout << "pendingSize: " << _pendingSerializationQueue.size() << endl;

	auto& replicationManager = ReplicationManager::GetInstance();
	OutputMemoryBitStream outStream;

	outStream.WriteBits(static_cast<int>(PacketType::PT_ReplicationData),
		GetRequiredBits(static_cast<int>(PacketType::PT_Max)));

	// delta�� �ִ� ��ü�� Update �ϰ� �ʹ�.
	// => Update�� �ִ� ��ü�� GUID�� ����� Queue�� ����
	// ť�� ���Ҹ� ��� pop �ϸ鼭 stream�� ���
	// PacketQueue�� ���� �� stream ���� ����Ǳ� ������, ���⼭ stream�� �����ϰ� �Ҹ���ѵ�
	// ���� ����.
	int pendingSerializationCount = static_cast<int>(_pendingSerializationQueue.size());
	while (pendingSerializationCount--)
	{
		NetworkId_t networkId = _pendingSerializationQueue.front();
		_pendingSerializationQueue.pop();

		auto gameObject = _linkingContext.GetGameObject(networkId);
		replicationManager.ReplicateUpdate(outStream, gameObject);
		
		_updatedObjectNetworkIds.erase(networkId);
	}

	if (outStream.GetBitLength() <= 0)
		return;

	cout << "outStream.GetBitLength(): " << outStream.GetBitLength() << endl;
	cout << "outStream.GetByteLength(): " << outStream.GetByteLength() << endl;

	Packet packet{ outStream.GetBufferPtr(), outStream.GetByteLength() };

	packet.PrintInHex();

	auto& sendQueue = PacketQueue::GetSendStaticInstance();
	sendQueue.PushCopy(packet);
}

WorldServer::WorldServer() :
	_linkingContext(LinkingContext::GetInstance())
{ 
	_lastFixedUpdateTime = _lastPacketUpdateTime = system_clock::now();
}
