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

#include "inet/common/ModuleAccess.h"
#include "inet/common/queueing/PacketDelayer.h"

namespace inet {
namespace queueing {

Define_Module(PacketDelayer);

void PacketDelayer::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        inputGate = gate("in");
        producer = dynamic_cast<IPacketProducer *>(findConnectedModule(inputGate));
        outputGate = gate("out");
        consumer = check_and_cast<IPacketConsumer *>(getConnectedModule(outputGate));
    }
}

void PacketDelayer::handleMessage(cMessage *message)
{
    if (message->isSelfMessage()) {
        auto packet = check_and_cast<Packet *>(message);
        pushOrSendPacket(packet, outputGate, consumer);
    }
    else
        throw cRuntimeError("Unknown message");
}

void PacketDelayer::pushPacket(Packet *packet, cGate *gate)
{
    Enter_Method_Silent();
    EV_INFO << "Delaying packet " << packet->getName() << "." << endl;
    take(packet);
    packet->setArrival(getId(), inputGate->getId(), simTime());
    scheduleAt(simTime() + par("delay"), packet);
}

void PacketDelayer::handleCanPushPacket(cGate *gate)
{
    if (producer != nullptr)
        producer->handleCanPushPacket(inputGate);
}

} // namespace queueing
} // namespace inet

