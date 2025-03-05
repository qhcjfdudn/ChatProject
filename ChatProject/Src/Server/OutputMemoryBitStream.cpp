#include "ServerPCH.h"
#include "OutputMemoryBitStream.h"

void OutputMemoryBitStream::ReallocBuffer(uint32_t newBitCapacity) {
	char* newBuffer = new char[newBitCapacity];

	memcpy(newBuffer, _buffer, _bitHead);

	free(_buffer);

	_buffer = newBuffer;
}


void OutputMemoryBitStream::WriteBits(uint8_t inData, size_t inBitCount)
{
	// inBitCount�� ���� buffer�� ����� �÷����� �Ǵ�
	uint32_t nextBitHead = _bitHead + static_cast<uint32_t>(inBitCount);
	if (nextBitHead > _bitCapacity) {
		ReallocBuffer(max(_bitCapacity * 2, nextBitHead));
	}

	uint32_t byteOffset = _bitHead >> 3;
	uint32_t bitOffset = _bitHead & 7;

	// ���� ó�� �� ����Ʈ�� inData �� �� �� �ִ� ��ŭ �켱 ����
	uint8_t currentMask = ~(0xff << bitOffset);
	_buffer[byteOffset] = (_buffer[byteOffset] & currentMask) | (inData << bitOffset);

	// inData�� ���� ���� ���� �κ� ����
	uint32_t bitsFreeThisByte = 8 - bitOffset;
	if (inBitCount > bitsFreeThisByte) {
		_buffer[byteOffset + 1] = inData >> bitsFreeThisByte;
	}

	_bitHead = nextBitHead;
}

void OutputMemoryBitStream::WriteBits(const void* inData, size_t inBitCount)
{
	const char* srcByte = static_cast<const char*>(inData);

	while (inBitCount > 8) {
		WriteBits(*srcByte, 8);
		srcByte++;
		inBitCount -= 8;
	}

	if (inBitCount > 0) {
		WriteBits(*srcByte, inBitCount);
	}
}

void OutputMemoryBitStream::Write(uint32_t inData)
{
	WriteBits(&inData, sizeof(uint32_t) << 3);
}

void OutputMemoryBitStream::Write(std::string inData)
{

}

void OutputMemoryBitStream::Write(Vector2 inData)
{
}