//
// Copyright (C) 2019 Oren Zaharia @ Ben-Gurion University
// Copyright (C) 2017 Jan Peter Drees
// Copyright (C) 2015 Joaquim Oller
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

package inet.linklayer.rimac;

import inet.common.queueing.contract.IPacketQueue;
import inet.linklayer.base.MacProtocolBase;
import inet.linklayer.contract.IMacProtocol;

//
// Implementation of RI-MAC.
// See C++ documentation for details.
//
module RIMac extends MacProtocolBase like IMacProtocol
{
    parameters:
        string address = default("auto"); // MAC address as hex string (12 hex digits), or
                                          // "auto". "auto" values will be replaced by
                                          // a generated MAC address in init stage 0.
		
        // debug switch
        bool debug = default(false);
        //collect statistics?
        bool stats = default(true);

        // RIMAC specific parameters
        // how long is one slot?
        double maxRange @unit(m) = default(550m);
        // how long is one slot?
        double sleepInterval @unit(s) = default(1s);
        
        // mini slot
        double slotDuration @unit(s) = default(0.00032s);

        // how long is the check interval (CCA)?
        double checkInterval @unit(s) = default(0.000128s);
        
        // how long is the check interval (CCA)?
        double clearChannelAssessment @unit(s) = default(0.000128s);
        
        // how long is the check interval (CCA)?
        double sifs @unit(s) = default(0.000192s);
        
        // size of the MAC queue (maximum number of packets in Tx buffer)
        int queueLength = default(20);

        // should we animate the nodes with colors depending on their state or 
        // not?
        bool animation = default(true);

        // bit rate 
        double bitrate @unit(bps) = default(250000 bps); //250Kbps

        //tx power
        double txPower @unit(W) = default(50 mW);

        //should mac send and expect acknowledgments?
        bool useMACAcks = default(false);

        // maximum number of frame retransmission
        // only used when usage of MAC acks is enabled
        int macMaxFrameRetries = default(3);

        // length of MAC header
        int headerLength @unit(b) = default(6b);
        int mtu @unit(B) = default(0B);

        // Maximum transmission attempts per data packet
        int maxTransmissions = default(3);
        int minWindowSize = default(2);
        int maxWindowSize = default(8);

        string radioModule = default("^.radio");   // The path to the Radio module  //FIXME remove default value 

        @class(RIMac);
        @signal[packetDropped](type=inet::Packet);
    submodules:
        queue: <default("DropTailQueue")> like IPacketQueue {
            parameters:
                frameCapacity = default(queueLength);
                @display("p=100,100;q=l2queue");
        }
    connections allowunconnected:
}

