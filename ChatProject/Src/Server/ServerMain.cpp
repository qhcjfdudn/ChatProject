#include "ServerPCH.h"

#include <csignal>

#include "Engine.h"
#include "NetworkManagerServer.h"
#include "WorldServer.h"
#include "Constant.h"

void signalHandler(int signum)
{
	cout << "\nInterrupt signal (" << signum << ") received." << endl;
	Engine& engineInstance = Engine::GetInstance();
	engineInstance.TurnOff();
}

int main()
{
	signal(SIGINT, signalHandler);
	
	auto& engineInstance = Engine::GetInstance();
	engineInstance.initPhysics();

	auto& networkInstance = NetworkManagerServer::GetInstance();
	networkInstance.InitIOCP();

	auto& worldInstance = WorldServer::GetInstance();
	worldInstance.InitLevel();

	thread userInputThread([] {
		auto& engineInstance = Engine::GetInstance();
		string cmd;
		while (engineInstance.isRunning)
		{
			std::getline(std::cin, cmd);
			if (cmd == "r")
			{
				WorldServer::GetInstance().RemoveAllGameObjects();
			}
			else if (cmd == "s")
			{
				WorldServer::GetInstance().InitLevel();
			}
		}
		});

	// fps ������Ʈ�� ���� ���� thread ����
	// �����͸� ���� �ʿ�
	thread physXThread([]() {
		auto& engineInstance = Engine::GetInstance();
		system_clock::time_point _lastFixedUpdateTime = system_clock::now();

		// thread ������ �����ϴ� �ܺ� ����. atomic���� ������ ������ ���ü� ���� �ذ�����.
		while (engineInstance.isRunning)
		{
			system_clock::time_point currentTime = system_clock::now();
			std::chrono::duration<double> elapsedTime = currentTime - _lastFixedUpdateTime;

			if (elapsedTime.count() < Constant::PHYSX_FIXED_UPDATE_TIMESTEP)
				continue;

			engineInstance.stepPhysics();

			_lastFixedUpdateTime = currentTime;
		}
		});

	while (engineInstance.isRunning) {
		networkInstance.ProcessIOCPEvent();
		worldInstance.Update();
		worldInstance.FixedUpdate();
		worldInstance.WriteWorldStateToStream();
		networkInstance.SendPacketsIOCP();
	}

	physXThread.join();
	userInputThread.join();

	worldInstance.Clear();
	engineInstance.cleanupPhysics();
	
	cout << "Server Main done." << endl;
}
