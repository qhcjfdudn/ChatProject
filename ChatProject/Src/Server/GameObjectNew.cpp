#include "ServerPCH.h"
#include "GameObjectNew.h"

#include "Constant.h"
#include "OutputMemoryBitStream.h"
#include "EngineNew.h"
#include "Level.h"

GameObjectNew::GameObjectNew(PxVec2 location, PxVec2 velocity) :
	_location(location), _velocity(velocity) {
}

void GameObjectNew::SetDirtyFlag(ReplicationFlag flag)
{
	replicationFlag |= flag;
}

void GameObjectNew::SetVelocity(PxVec2 velocity)
{
	_velocity = velocity;
}

bool GameObjectNew::FixedUpdate()
{
	SetCurrentTransform();

	return true;
}

void GameObjectNew::SetCurrentTransform()
{
	// simulate �߿��� getGlobalPose()�� ����� �� ���ٴ� ���� �޽��� �߰�
	// ���� ������ ������ ���� �˾ƿ����� �� ���� lockRead()�� �ɾ�� �Ѵ�.

	PxVec3 curLocation{ _rigidbody->getGlobalPose().p };
	PxVec3 curVelocity{ _rigidbody->getLinearVelocity() };

	_location = PxVec2{ curLocation.x, curLocation.y };
	_velocity = PxVec2{ curVelocity.x, curVelocity.y };
}

unsigned int GameObjectNew::GetClassId()
{
	return 'GMOJ';
}

void GameObjectNew::Write(OutputMemoryBitStream& inStream)
{
	inStream.Write(_location);
	inStream.Write(_velocity);
}

PxActor* GameObjectNew::GetActor() const
{
	return _rigidbody;
}

void GameObjectNew::SetRigidbody(PxRigidDynamic& rigidbody)
{
	_rigidbody = &rigidbody;
}
