#include "ServerPCH.h"
#include "Observer.h"

void Observer::notify(ObserverEvent oe)
{
	switch (oe)
	{
	case ObserverEvent::EngineOff:
		// ���� �ִ� thread�� �ִٸ� ���⿡ �߰��� ���� ����
		return;
	}
}
