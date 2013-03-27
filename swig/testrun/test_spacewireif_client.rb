require "SpaceWireRMAP"
include SpaceWireRMAP


serverURL="localhost"
tcpPort=10030
spwif=SpaceWireIFOverTCP.new(serverURL,tcpPort)

spwif.open()
print "Connected\n"

packet=VectorUInt8.new([1,2,3,4,5,6,7])
spwif.send(packet)

print "Sent\n"

spwif.close()
