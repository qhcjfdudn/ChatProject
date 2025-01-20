#pragma once
class ReceiveQueue
{
public:
	static ReceiveQueue& GetInstance();

	void Push(const string& str);
	string Front();
	bool Empty();

private:
	ReceiveQueue() {}
	~ReceiveQueue() {}

	// NetworkManager�� World�� ���� �ٸ� thread���� �������״�
	// lock/unlock ��� �ʿ�
	std::queue<string> _bufferQueue;
};

// stream ��ü�� �����Ѵٸ�?
// �ϴ� string �������� queue�� ����� ���� stream Ÿ������ ����