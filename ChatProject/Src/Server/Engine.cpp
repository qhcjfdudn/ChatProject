#include "ServerPCH.h"
#include "Engine.h"

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

	CreatePlain(0.f, 1.f, 0.f, 0.f);

	for (PxU32 i = 0; i < 5; i++)
		createStack(PxTransform(PxVec3(0, 0, stackZ -= 10.0f)), 10, 2.0f);

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
	// FixedUpdate ����
	pxScene->simulate(1.0f / 60.0f);
	pxScene->fetchResults(true);
}

void Engine::CreatePlain(float nx, float ny, float nz, float distance)
{
	pxMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*pxPhysics, PxPlane(nx, ny, nz, distance), *pxMaterial);
	pxScene->addActor(*groundPlane);
}

PxRigidDynamic* Engine::createDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity)
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*pxPhysics, t, geometry, *pxMaterial, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	pxScene->addActor(*dynamic);
	return dynamic;
}

void Engine::createStack(const PxTransform& t, PxU32 size, PxReal halfExtent)
{
	PxShape* shape = pxPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *pxMaterial);
	for (PxU32 i = 0; i < size; i++)
	{
		for (PxU32 j = 0; j < size - i; j++)
		{
			PxTransform localTm(PxVec3(PxReal(j * 2) - PxReal(size - i), PxReal(i * 2 + 1), 0) * halfExtent);
			PxRigidDynamic* body = pxPhysics->createRigidDynamic(t.transform(localTm));
			body->attachShape(*shape);
			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			pxScene->addActor(*body);
		}
	}
	shape->release();
}