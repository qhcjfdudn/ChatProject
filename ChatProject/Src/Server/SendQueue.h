#pragma once
class SendQueue
{
public:
	static SendQueue& GetInstance();

	void Push(const string& str);
	string Front();
	bool Empty();

private:
	SendQueue() {}
	~SendQueue() {}

	// NetworkManager�� World�� ���� �ٸ� thread���� �������״�
	// lock/unlock ��� �ʿ�
	std::queue<string> _bufferQueue;
};

