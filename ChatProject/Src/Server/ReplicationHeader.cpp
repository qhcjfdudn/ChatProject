#include "ServerPCH.h"
#include "ReplicationHeader.h"

void ReplicationHeader::Write(OutputMemoryBitStream& inStream)
{
	inStream.WriteBits(&_ra, static_cast<int>(ReplicationAction::RA_Max));
	inStream.Write(_nid);
	inStream.Write(_cid);
}
