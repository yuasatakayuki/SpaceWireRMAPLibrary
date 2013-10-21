require "SpaceWireRMAP"
include SpaceWireRMAP
p=RMAPPacket.new

targetPathAddress=VectorUInt8.new
targetPathAddress.push(0x03)
targetPathAddress.push(0x02) 
targetPathAddress.push(0x0a)

p.setTargetSpaceWireAddress(targetPathAddress)
p.setReplyAddress(targetPathAddress)

p.setCommand()
p.setWrite()
p.unsetVerifyFlag()

writeData=VectorUInt8.new
writeData.push(0x00)
writeData.push(0x00)
writeData.push(0x20)
writeData.push(0x00)

p.setData(writeData)


p.constructPacket()

puts "============================================"
puts "Byte dump (RMAPPacket.getPacketBufferPointer()"
puts "============================================"
p.getPacketBufferPointer().each(){|e|
 print "%02x " % e
}
puts ""
puts ""
puts ""
puts ""
	
	

puts "============================================"
puts "RMAPPacket.toSrring()"
puts "============================================"
print p.toString()


