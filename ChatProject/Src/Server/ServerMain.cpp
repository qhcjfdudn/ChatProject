#include "ServerPCH.h"

#include "Engine.h"
#include "NetworkManagerServer.h"
#include "WorldServer.h"

int main() {
	auto& engine = Engine::GetInstance();
	
	auto& networkInstance = NetworkManagerServer::GetInstance();
	//networkInstance.Init();
	networkInstance.InitIOCP();

	auto& worldInstance = WorldServer::GetInstance();

	cout << "engine start" << endl;
	while (engine.isRunning) {

		//networkInstance.ReceivePackets();
		networkInstance.ProcessIOCPEvent();
		worldInstance.Update();
		//networkInstance.SendPackets();
		networkInstance.SendPacketsIOCP();

		//engine.isRunning = false;
	}
	cout << "engine end" << endl;
}