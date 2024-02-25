//
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.linklayer.common.MacAddress;

class LocalBroadcastHeader extends inet::FieldsChunk
{
    uint16_t    magicNo;             // a magic number identifying local broadcasts
    uint16_t    version;             // version of the local broadcast protocol
    uint32_t    seqno;               // sequence number
    inet::MacAddress  senderId;      // Mac address of sender, used as sender id
}
