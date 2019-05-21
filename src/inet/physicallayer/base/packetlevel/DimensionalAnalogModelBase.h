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

#ifndef __INET_DIMENSIONALANALOGMODELBASE_H
#define __INET_DIMENSIONALANALOGMODELBASE_H

#include "inet/common/math/Function.h"
#include "inet/physicallayer/base/packetlevel/AnalogModelBase.h"
#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"

namespace inet {

namespace physicallayer {

class AttenuationFunction : public math::Function<double, simtime_t, Hz>
{
  protected:
    const IRadioMedium *radioMedium = nullptr;
    const double transmitterAntennaGain;
    const double receiverAntennaGain;
    const Coord transmissionPosition;
    const Coord receptionPosition;
    m distance;

  public:
    AttenuationFunction(const IRadioMedium *radioMedium, const double transmitterAntennaGain, const double receiverAntennaGain, const Coord transmissionPosition, const Coord receptionPosition) :
        radioMedium(radioMedium), transmitterAntennaGain(transmitterAntennaGain), receiverAntennaGain(receiverAntennaGain), transmissionPosition(transmissionPosition), receptionPosition(receptionPosition)
    {
        distance = m(transmissionPosition.distance(receptionPosition));
    }

    virtual double getValue(const math::Point<simtime_t, Hz>& p) const override {
        auto frequency = std::get<1>(p);
        auto propagationSpeed = radioMedium->getPropagation()->getPropagationSpeed();
        auto pathLoss = radioMedium->getPathLoss()->computePathLoss(propagationSpeed, frequency, distance);
        // TODO: obstacle loss is time dependent
        auto obstacleLoss = radioMedium->getObstacleLoss() ? radioMedium->getObstacleLoss()->computeObstacleLoss(frequency, transmissionPosition, receptionPosition) : 1;
        return std::min(1.0, transmitterAntennaGain * receiverAntennaGain * pathLoss * obstacleLoss);
    }

    virtual void iterateInterpolatable(const math::Interval<simtime_t, Hz>& i, const std::function<void (const math::Interval<simtime_t, Hz>& i)> f) const override {
        // TODO: this function is continuous in frequency, so what should we do?
        f(i);
    }
};

class INET_API DimensionalAnalogModelBase : public AnalogModelBase
{
  protected:
    bool attenuateWithCarrierFrequency;

  protected:
    virtual void initialize(int stage) override;

  public:
    virtual std::ostream& printToStream(std::ostream& stream, int level) const override;

    virtual const math::Function<W, simtime_t, Hz> *computeReceptionPower(const IRadio *radio, const ITransmission *transmission, const IArrival *arrival) const;
    virtual const INoise *computeNoise(const IListening *listening, const IInterference *interference) const override;
    virtual const INoise *computeNoise(const IReception *reception, const INoise *noise) const override;
    virtual const ISnir *computeSNIR(const IReception *reception, const INoise *noise) const override;
};

} // namespace physicallayer

} // namespace inet

#endif // ifndef __INET_DIMENSIONALANALOGMODELBASE_H

