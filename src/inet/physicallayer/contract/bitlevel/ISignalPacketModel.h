//
// Copyright (C) 2013 OpenSim Ltd.
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

#ifndef __INET_ISIGNALPACKETMODEL_H
#define __INET_ISIGNALPACKETMODEL_H

#include "inet/common/packet/Packet.h"
#include "inet/physicallayer/contract/bitlevel/IFecCoder.h"
#include "inet/physicallayer/contract/bitlevel/IInterleaver.h"
#include "inet/physicallayer/contract/bitlevel/IScrambler.h"
#include "inet/physicallayer/contract/packetlevel/IPrintableObject.h"

namespace inet {

namespace physicallayer {

/**
 * This purely virtual interface provides an abstraction for different radio
 * signal models in the packet domain.
 */
class INET_API ISignalPacketModel : public IPrintableObject
{
  public:
    virtual const Packet *getPacket() const = 0;
    /**
     * Returns the net bitrate (datarate) of the packet.
     */
    virtual bps getBitrate() const = 0;
};

class INET_API ITransmissionPacketModel : public virtual ISignalPacketModel
{
};

class INET_API IReceptionPacketModel : public virtual ISignalPacketModel
{
};

} // namespace physicallayer

} // namespace inet

#endif // ifndef __INET_ISIGNALPACKETMODEL_H

