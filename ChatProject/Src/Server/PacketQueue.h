#pragma once
class PacketQueue
{
public:
	static PacketQueue& GetInstance();

	void Push(const string& str);
	string Front();
	bool Empty();

private:
	PacketQueue() {}
	~PacketQueue() {}

	// NetworkManager�� World�� ���� �ٸ� thread���� �������״�
	// lock/unlock ��� �ʿ�
	std::queue<string> _bufferQueue;
};

// stream ��ü�� �����Ѵٸ�?
// �ϴ� string �������� queue�� ����� ���� stream Ÿ������ ����