#include "ServerPCH.h"

#include <csignal>

#include "Engine.h"
#include "NetworkManagerServer.h"

#include "Constant.h"
#include "Level.h"

void signalHandler(int signum)
{
	cout << "\nInterrupt signal (" << signum << ") received." << endl;
	auto& engineInstance = Engine::GetInstance();
	engineInstance.TurnOff();
}

int main()
{
	signal(SIGINT, signalHandler);
	
	auto& engineInstance = Engine::GetInstance();
	engineInstance.initPhysics();

	auto& networkInstance = NetworkManagerServer::GetInstance();
	networkInstance.InitIOCP();

	vector<Level> levels(1);
	levels[0].InitLevel();

	thread userInputThread([&levels] {
		auto& engineInstance = Engine::GetInstance();
		string cmd;
		while (engineInstance.isRunning)
		{
			std::getline(std::cin, cmd);
			if (cmd == "r")
			{
				for (auto& level : levels)
					level.RemoveAllGameObjects();
			}
			else if (cmd == "s")
			{
				for (auto& level : levels)
					level.InitLevel();
			}
		}
		});

	// fps ������Ʈ�� ���� ���� thread ����
	// �����͸� ���� �ʿ�
	thread physXThread([&levels]() {
		auto& engineInstance = Engine::GetInstance();

		// thread ������ �����ϴ� �ܺ� ����. atomic���� ������ ������ ���ü� ���� �ذ�����.
		while (engineInstance.isRunning)
		{
			for (auto& level : levels)
			{
				system_clock::time_point currentTime = system_clock::now();
				std::chrono::duration<double> elapsedTime = currentTime - level.lastPhysxFixedUpdateTime;

				if (elapsedTime.count() < Constant::PHYSX_FIXED_UPDATE_TIMESTEP)
					continue;

				level.lastPhysxFixedUpdateTime = currentTime;

				level.StepPhysics();
			}
		}
		});

	// Networking ���۰� world ������ ���� thread�� �и� �ʿ�
	while (engineInstance.isRunning) {
		networkInstance.ProcessIOCPEvent();
		
		for (auto& level : levels) {
			level.Update();
			level.FixedUpdate();
			level.WriteWorldStateToStream();
		}

		networkInstance.SendPacketsIOCP();
	}

	physXThread.join();
	userInputThread.join();

	for (auto& level : levels)
		level.Release();
	
	engineInstance.cleanupPhysics();
	
	cout << "Server Main done." << endl;
}
