#include "ServerPCH.h"
#include "GameObject.h"

#include "OutputMemoryBitStream.h"

GameObject::GameObject(PxVec2 location, PxVec2 velocity) :
	_location(location), _velocity(velocity) {
}

GameObject::~GameObject()
{
	if (_rigidbody && _rigidbody->isReleasable())
		_rigidbody->release();
}

void GameObject::SetDirtyFlag(ReplicationFlag flag)
{
	replicationFlag |= flag;
}

void GameObject::SetVelocity(PxVec2 velocity)
{
	_velocity = velocity;
}

void GameObject::SetRigidbody(PxRigidDynamic& rigidbody)
{
	_rigidbody = &rigidbody;
}

bool GameObject::FixedUpdate()
{
	SetCurrentTransform();

	return true;
}

void GameObject::SetCurrentTransform()
{
	// simulate 중에는 getGlobalPose()를 사용할 수 없다는 에러 메시지 발견
	// 물리 엔진에 접근해 값을 알아오고자 할 때는 lockRead()를 걸어야 한다.

	PxVec3 curLocation{ _rigidbody->getGlobalPose().p };
	PxVec3 curVelocity{ _rigidbody->getLinearVelocity() };

	_location = PxVec2{ curLocation.x, curLocation.y };
	_velocity = PxVec2{ curVelocity.x, curVelocity.y };
}

unsigned int GameObject::GetClassId()
{
	return 'GMOJ';
}

void GameObject::Write(OutputMemoryBitStream& inStream)
{
	inStream.Write(_location);
	inStream.Write(_velocity);
}

PxActor* GameObject::GetRigidbody() const
{
	return _rigidbody;
}
