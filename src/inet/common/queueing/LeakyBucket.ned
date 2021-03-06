//
// Copyright (C) OpenSim Ltd.
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
// along with this program; if not, see http://www.gnu.org/licenses/.
//

package inet.common.queueing;

import inet.common.queueing.contract.IPacketQueue;
import inet.common.queueing.contract.IPacketServer;

module LeakyBucket like IPacketQueue
{
    parameters:
        @display("i=block/buffer");
    gates:
        input in @labels(push);
        output out @labels(pop);
    submodules:
        queue: <default("DropTailQueue")> like IPacketQueue {
            parameters:
                @display("p=100,120");
        }
        server: <default("PacketServer")> like IPacketServer {
            parameters:
                @display("p=220,120");
        }
    connections:
        in --> queue.in;
        queue.out --> server.in;
        server.out --> out;
}

