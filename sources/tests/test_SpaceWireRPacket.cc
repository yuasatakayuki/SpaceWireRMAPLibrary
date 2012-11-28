#include "SpaceWireR/SpaceWireRPacket.hh"
int main(int argc, char* argv[]){
SpaceWireRPacket packet;

std::vector<uint8_t> payload;
for(size_t i=0;i<10;i++){
 payload.push_back(i);
}

packet.setPayload(payload);
SpaceWireUtilities::dumpPacket(packet.getPacketAsVectorPointer());

}
