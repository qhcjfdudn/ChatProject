#include "ServerPCH.h"
#include "Engine.h"

#include "Constant.h"

Engine& Engine::GetInstance() {
	static Engine instance;
	return instance;
}

void Engine::TurnOff()
{
	isRunning = false;
	observer.notify(ObserverEvent::EngineOff);
}

void Engine::initPhysics()
{
	pxFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, pxAllocator, pxErrorCallback);

	pxPvd = PxCreatePvd(*pxFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	pxPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	pxPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *pxFoundation, PxTolerancesScale(), true, pxPvd);

	PxSceneDesc sceneDesc(pxPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	pxDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = pxDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	pxScene = pxPhysics->createScene(sceneDesc);

	PxPvdSceneClient* pvdClient = pxScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	cout << "initPhysics done." << endl;
}

void Engine::cleanupPhysics()
{
	PX_RELEASE(pxScene);
	PX_RELEASE(pxDispatcher);
	PX_RELEASE(pxPhysics);
	if (pxPvd)
	{
		PxPvdTransport* transport = pxPvd->getTransport();
		PX_RELEASE(pxPvd);
		PX_RELEASE(transport);
	}
	PX_RELEASE(pxFoundation);

	cout << "cleanupPhysics done." << endl;
}

void Engine::stepPhysics()
{
	pxScene->lockWrite();
	pxScene->simulate(Constant::PHYSX_FIXED_UPDATE_TIMESTEP);
	pxScene->fetchResults(true);
	pxScene->unlockWrite();
}

void Engine::RemoveActor(PxActor* actor)
{
	if (actor == nullptr || actor->isReleasable() == false)
		return;

	cout << "RemoveActor" << endl;
	pxScene->removeActor(*actor);
	actor->release(); // �޸� ����
}

void Engine::LockWrite()
{
	pxScene->lockWrite();
}

void Engine::UnlockWrite()
{
	pxScene->unlockWrite();
}

void Engine::LockRead()
{
	pxScene->lockRead();
}

void Engine::UnlockRead()
{
	pxScene->unlockRead();
}

void Engine::CreatePlain(float nx, float ny, float nz, float distance)
{
	pxMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*pxPhysics, PxPlane(nx, ny, nz, distance), *pxMaterial);
	pxScene->lockWrite();
	pxScene->addActor(*groundPlane);
	pxScene->unlockWrite();
}

PxRigidDynamic* Engine::CreateBox(const PxTransform& tp, float halfExtentX, float halfExtentY, float halfExtentZ)
{
	PxRigidDynamic* body = pxPhysics->createRigidDynamic(tp);

	PxShape* shape = pxPhysics->createShape(PxBoxGeometry(halfExtentX, halfExtentY, halfExtentZ), *pxMaterial);
	body->attachShape(*shape);

	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);

	pxScene->lockWrite();
	pxScene->addActor(*body);
	pxScene->unlockWrite();

	shape->release();

	return body;
}

PxRigidDynamic* Engine::CreateBox2D(const PxVec2& location, float halfExtentX, float halfExtentY)
{
	PxRigidDynamic* body = pxPhysics->createRigidDynamic(PxTransform{ location.x, location.y, 0 });

	PxShape* shape = pxPhysics->createShape(PxBoxGeometry(halfExtentX, halfExtentY, 0.1f), *pxMaterial);
	body->attachShape(*shape);

	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);

	pxScene->lockWrite();
	pxScene->addActor(*body);
	pxScene->unlockWrite();

	shape->release();

	return body;
}

void Engine::CreateBox2DStatic(const PxVec2& location, float halfExtentX, float halfExtentY)
{
	PxRigidStatic* body = pxPhysics->createRigidStatic(PxTransform{ location.x, location.y, 0 });

	PxShape* shape = pxPhysics->createShape(PxBoxGeometry(halfExtentX, halfExtentY, 0.1f), *pxMaterial);
	body->attachShape(*shape);
	
	pxScene->lockWrite();
	pxScene->addActor(*body);
	pxScene->unlockWrite();

	shape->release();
}

PxRigidDynamic* Engine::CreateSphere2D(const PxVec2& location, float halfExtentRadius)
{
	PxRigidDynamic* body = pxPhysics->createRigidDynamic(PxTransform{ location.x, location.y, 0 });
	PxShape* shape = pxPhysics->createShape(PxSphereGeometry(halfExtentRadius), *pxMaterial);
	body->attachShape(*shape);

	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	
	pxScene->lockWrite();
	pxScene->addActor(*body);
	pxScene->unlockWrite();

	shape->release();

	return body;
}

PxRigidDynamic* Engine::createDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity)
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*pxPhysics, t, geometry, *pxMaterial, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);

	pxScene->lockWrite();
	pxScene->addActor(*dynamic);
	pxScene->unlockWrite();

	return dynamic;
}
