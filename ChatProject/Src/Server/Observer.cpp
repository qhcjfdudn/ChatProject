#include "ServerPCH.h"
#include "Observer.h"

void Observer::notify(const ObserverEvent& oe) const
{
	switch (oe)
	{
	case ObserverEvent::EngineOff:
		// ���� �ִ� thread�� �ִٸ� ���⿡ �߰��� ���� ����
		return;
	}
}
