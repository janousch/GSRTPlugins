
var DEFAULT_OPCODE = 1;
var CLOCK_SYNC_OPCODE = 2;
var SERVER_TIME_OPCODE = 3;

var peerIds = [];

/**Object for getting the packetService functions and their auto complete*/
var packetService = {
    WRAPPER_PACKET_CODE: 127,
    
    /**
     * Replicates the packet to all clients except the sender.
     * @param {RTPacket} packet
     * ->
     */
    replicatePacketToTargetClients: function(packet) {
        if (peerIds.length > 1) {
            var sender = packet.getSender().getPeerId();
            var targetPeers = [];
            if (packet.getTargetPlayers().length === 0) {
                // Broadcast
                targetPeers = [].concat(peerIds);
                
                var peerIndex = peerIds.indexOf(sender, 0);
                if (peerIndex > -1) {
                    targetPeers.splice(peerIndex, 1);
                }
            } else {
                // Uni- or Multicast
                targetPeers = packet.getTargetPlayers();
            }
            
            
            var data = packet.getData();
            var packetData = RTSession.newData().setData(packetService.WRAPPER_PACKET_CODE, data);
            
            var packetForClients = RTSession.newPacket().setOpCode(DEFAULT_OPCODE);
            packetForClients.setData(packetData);
            packetForClients.setTargetPeers(targetPeers);
            packetForClients.setReliable(true);
            packetForClients.setSender(sender);
            packetForClients.send();
        }
    }
};

RTSession.onPlayerConnect(function(player) {
    peerIds.push(player.getPeerId());
    
    if (peerIds.length == 2) {
        RTSession.setInterval(function(){ // send current server time to all players every 1second
            //RTSession.getLogger().debug(new Date().getTime());
            RTSession.newPacket().setOpCode(SERVER_TIME_OPCODE).setTargetPeers().setData( RTSession.newData().setFloat(1, new Date().getTime() )).send();
        }, 1000);
    }
});

RTSession.onPlayerDisconnect(function(player) {
    var peerIndex = peerIds.indexOf(player.getPeerId(), 0);
    if (peerIndex > -1) {
        peerIds.splice(peerIndex, 1);
    }
});


RTSession.onPacket(DEFAULT_OPCODE, function(packet) {
    packetService.replicatePacketToTargetClients(packet);
});

// packet CLOCK_SYNC_OPCODE is a timestamp from the client for clock-syncing
RTSession.onPacket(CLOCK_SYNC_OPCODE, function(packet){
    var rtData = RTSession.newData().setFloat(1, packet.getData().getFloat(1)) // return the timestamp the server just got
                                    .setFloat(2, new Date().getTime()) // return the current time on the server
                                    
    RTSession.newPacket().setData(rtData).setOpCode(CLOCK_SYNC_OPCODE).setTargetPeers(packet.getSender().getPeerId()).send(); // send the packet back to the peer that sent it
    // we've also set this packet to be op-code CLOCK_SYNC_OPCODE.
    // we used CLOCK_SYNC_OPCODE to send the packet, but we only ever send the packet from client-to-server
});