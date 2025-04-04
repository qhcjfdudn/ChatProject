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

void WorldServer::Clear()
{
	RemoveAllGameObjects();

	_pendingSerializationQueue = queue<shared_ptr<GameObject>>{};

	_linkingContext.Clear();
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

void WorldServer::RemoveAllGameObjects() {
	for (int idx = static_cast<int>(_gameObjects.size()) - 1; idx >= 0; --idx)
		RemoveGameObject(idx);
}

// World�� �����ϴ� gameObject�� �����. O(N)�̶� gameObjects�� �������� ���� ���� �߻�
void WorldServer::Remove(shared_ptr<GameObject> gameObject) {
	size_t idx = std::find(_gameObjects.begin(), _gameObjects.end(), gameObject) - _gameObjects.begin();
	
	if (idx >= _gameObjects.size())
		return;

	RemoveGameObject(idx);
}

void WorldServer::RemoveGameObject(size_t idx)
{
	auto& go = _gameObjects[idx];

	if (hasFlag(go->dirtyFlag, DirtyFlag::DF_ALL) == false)
		_pendingSerializationQueue.push(go);

	go->SetDirtyFlag(DirtyFlag::DF_DELETE);
	
	swap(go, _gameObjects.back());
	_gameObjects.pop_back();
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

	_lastFixedUpdateTime = currentTime;

	const local_time<system_clock::duration> now = zoned_time{ current_zone(), currentTime }.get_local_time();
	cout << "[" << now << "] FixedUpdate" << endl;

	// �Ʒ� �ڵ尡 �������� �����ϴ��� ���� �ʿ�
	// ex) _gameObjects�� ���� �� _gameObjects�� ����� �߰�/����/������ �߻��Ѵٸ�?
	auto gameObjects = _gameObjects;
	for (auto& gameObject : gameObjects)
	{
		if (gameObject->FixedUpdate() == false)
			continue;
		
		DirtyFlag& dirtyFlag = gameObject->dirtyFlag;

		if (hasFlag(dirtyFlag, (DirtyFlag::DF_UPDATE | DirtyFlag::DF_DELETE)))
			continue;

		gameObject->SetDirtyFlag(DirtyFlag::DF_UPDATE);
		_pendingSerializationQueue.push(gameObject);
	}
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

	OutputMemoryBitStream outStream;
	outStream.WriteBits(static_cast<int>(PacketType::PT_ReplicationData),
		GetRequiredBits(static_cast<int>(PacketType::PT_Max)));

	auto& replicationManager = ReplicationManager::GetInstance();
	int pendingSerializationCount = static_cast<int>(_pendingSerializationQueue.size());
	while (pendingSerializationCount--)
	{
		shared_ptr<GameObject> go = _pendingSerializationQueue.front();
		_pendingSerializationQueue.pop();

		DirtyFlag& dirtyFlag = go->dirtyFlag;

		if (hasFlag(dirtyFlag, DirtyFlag::DF_DELETE))
		{
			cout << "DF_DELETE" << endl;
			replicationManager.ReplicateDelete(outStream, go);
			_linkingContext.RemoveGameObject(go);
		}
		else if (hasFlag(dirtyFlag, DirtyFlag::DF_UPDATE))
		{
			replicationManager.ReplicateUpdate(outStream, go);
		}
		else if (hasFlag(dirtyFlag, DirtyFlag::DF_CREATE))
		{
			//replicationManager.ReplicateCreate(outStream, go);
			//_linkingContext.AddGameObject(go);
		}

		dirtyFlag = DirtyFlag::DF_NONE;
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
