#include "ServerPCH.h"

#include <csignal>

#include "Engine.h"
#include "NetworkManagerServer.h"
#include "WorldServer.h"

void signalHandler(int signum) {
	cout << "\nInterrupt signal (" << signum << ") received." << endl;
	Engine& engineInstance = Engine::GetInstance();
	engineInstance.TurnOff();
}

int main() {
	auto& engineInstance = Engine::GetInstance();
	engineInstance.initPhysics();

	auto& networkInstance = NetworkManagerServer::GetInstance();
	networkInstance.InitIOCP();

	auto& worldInstance = WorldServer::GetInstance();
	worldInstance.InitWorld();

	signal(SIGINT, signalHandler);

	while (engineInstance.isRunning) {
		engineInstance.stepPhysics();

		networkInstance.ProcessIOCPEvent();
		worldInstance.Update();
		worldInstance.FixedUpdate();
		worldInstance.WriteWorldStateToStream();
		networkInstance.SendPacketsIOCP();
	}

	engineInstance.cleanupPhysics();
	
	// ��� thread�� join �ʿ�
	cout << "Server Main done." << endl;
}
