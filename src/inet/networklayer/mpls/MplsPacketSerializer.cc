//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"
#include "inet/networklayer/mpls/MplsPacket_m.h"
#include "inet/networklayer/mpls/MplsPacketSerializer.h"

namespace inet {

Register_Serializer(MplsHeader, MplsPacketSerializer);

void MplsPacketSerializer::serialize(MemoryOutputStream& stream, const Ptr<const Chunk>& chunk) const
{
    const auto& mplsHeader = staticPtrCast<const MplsHeader>(chunk);
    size_t size = mplsHeader->getLabelsArraySize();
    for(uint8_t i = 0; i < size; ++i){
        auto& label = mplsHeader->getLabels(i);
        stream.writeNBitsOfUint64Be(label.getLabel(), 20);
        stream.writeNBitsOfUint64Be(label.getTc(), 3);
        stream.writeBit(i == size - 1);
        stream.writeByte(label.getTtl());
    }
    ASSERT(mplsHeader->getChunkLength() == B(4 * size));
}

const Ptr<Chunk> MplsPacketSerializer::deserialize(MemoryInputStream& stream) const
{
    auto mplsHeader = makeShared<MplsHeader>();
    if (B(stream.getRemainingLength()).get() > 0) {
        bool mbool = false;
        int i = 0;
        while(!mbool){
            mplsHeader->setLabelsArraySize(i + 1);
            MplsLabel* label = new MplsLabel();
            label->setLabel(stream.readNBitsToUint64Be(20));
            label->setTc(stream.readNBitsToUint64Be(3));
            mbool = stream.readBit();
            label->setTtl(stream.readByte());
            mplsHeader->setLabels(i, *label);
            mplsHeader->addChunkLength(B(4));
            ++i;
        }
    }
    return mplsHeader;
}

} // namespace inet


















