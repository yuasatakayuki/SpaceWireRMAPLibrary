#!/usr/bin/env ruby

#
# 2013-03-22 Hirokazu Odaka
# RMAP demo of the Ruby binding of SpaceWireRMAPLibrary
# - based on main_RMAP_readWireRMAPTargetNode.cc by Takayuki Yuasa
# 2013-11-24 Takayuki Yuasa
# - require and include sentences were updated

require "SpaceWireRMAP"
include SpaceWireRMAP

### initialize parameters
ConfigurationFile = "Tutorial_RMAPTarget.xml"
IPAddress = "localhost"
Port = 10030
TargetNodeID = "SampleRMAPTargetNode"
MemoryObjectID = "SampleRegister"

### load XML file
puts "Loading " + ConfigurationFile
db = RMAPTargetNodeDB.new
begin
  db.loadRMAPTargetNodesFromXMLFile(ConfigurationFile)
rescue => e
  puts "Error in loading " + ConfigurationFile
  puts e
  exit(-1)
end

### find RMAPTargetNode and MemoryObject 
target_node = nil
begin
  target_node = db.getRMAPTargetNode(TargetNodeID)
rescue => e
  puts "It seems RMAPTargetNode named " + TargetNodeID + " is not defined in " + ConfigurationFile
  puts e
	exit(-1)
else
  puts "RMAPTargetNode named " << TargetNodeID << " was found."
end

memory_object = nil
begin
  memory_object = target_node.getMemoryObject(MemoryObjectID)
rescue
  puts "It seems Mmeory Object named " + MemoryObjectID + " of " + TargetNodeID + " is not defined in " + ConfigurationFile
  puts e
  exit(-1)
else
  puts "Memory Object named " + MemoryObjectID + " was found."
end

### SpaceWire part
puts "Opening SpaceWireIF..."
spwif = SpaceWireIFOverTCP.new(IPAddress, Port)

begin
  spwif.open()
rescue
  puts "Connection timed out."
  puts e
  exit(-1)
end

### RMAPEngine/RMAPInitiator part
rmap_engine = RMAPEngine.new(spwif)
rmap_engine.start()
rmap_initiator = RMAPInitiator.new(rmap_engine)
rmap_initiator.setInitiatorLogicalAddress(0xFE)
rmap_initiator.setRMAPTargetNodeDB(db)

### Read
puts "Performing RMAP Read..."
buffer = nil # VectorUInt8.new
begin
  ### read
  buffer = rmap_initiator.read(TargetNodeID, MemoryObjectID, 300000.0)
rescue => ex
  puts "Exception => "
  puts ex
  exit(-1)
end

puts "Read successfully done."
puts "Data: "
buffer.each_with_index {|x, i| puts "%4d %4d" % [x, i]}

puts ""
puts "object => "
p buffer

puts "to_a => "
p buffer.to_a

### finalize
puts "RMAP enging stopping..."
rmap_engine.stop()
puts "SpaceWireIF closing..."
spwif.close()
