require "SpaceWireRMAP"
include SpaceWireRMAP
p=RMAPPacket.new
print p.toString()
print "\n"
print "#{p.isRead()}\n"

print "============================================\n"

n=RMAPTargetNode.new
print n.toString()
print "\n"
