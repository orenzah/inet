//
// Copyright (C) 2018 OpenSim Ltd.
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
// @author: Zoltan Bojthe
//

#include "inet/common/packet/dissector/ProtocolDissectorRegistry.h"
#include "inet/routing/ospfv2/OspfPacket_m.h"
#include "inet/routing/ospfv2/OspfProtocolDissector.h"


namespace inet {
namespace ospf {


Register_Protocol_Dissector(&Protocol::ospf, OspfProtocolDissector);

void OspfProtocolDissector::dissect(Packet *packet, const Protocol *protocol, ICallback& callback) const
{
    auto header = packet->popAtFront<OspfPacket>();
    callback.startProtocolDataUnit(&Protocol::ospf);
    callback.visitChunk(header, &Protocol::ospf);
    callback.endProtocolDataUnit(&Protocol::ospf);
}

} // namespace ospf
} // namespace inet

