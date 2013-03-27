require "SpaceWireRMAP"
include SpaceWireRMAP


tcpPort=10030
spwif=SpaceWireIFOverTCP.new(tcpPort)

print "Waiting for a client\n"
spwif.open()
print "Connected\n"

packet=VectorUInt8.new
spwif.receive(packet)
print "Received\n"
for n in packet 
 print "%02x " % n
end

spwif.close()
