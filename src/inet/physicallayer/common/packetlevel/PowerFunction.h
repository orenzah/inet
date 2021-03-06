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

#ifndef __INET_POWERFUNCTION_H_
#define __INET_POWERFUNCTION_H_

#include "inet/common/geometry/common/Coord.h"
#include "inet/common/math/Functions.h"
#include "inet/physicallayer/contract/packetlevel/IRadioMedium.h"

namespace inet {

namespace physicallayer {

using namespace inet::math;

/**
 * This mathematical function provides the signal attenuation for any given
 * time and frequency coordinates. This function assumes the transmitter and
 * the receiver are stationary for the duration of the transmission and the
 * reception. The result is not time dependent but this definition simplifies
 * further computations.
 */
class INET_API FrequencyAttenuationFunction : public FunctionBase<double, Domain<simtime_t, Hz>>
{
  protected:
    const IRadioMedium *radioMedium = nullptr;
    const double transmitterAntennaGain;
    const double receiverAntennaGain;
    const Coord transmissionPosition;
    const Coord receptionPosition;
    const m distance;

  public:
    FrequencyAttenuationFunction(const IRadioMedium *radioMedium, const double transmitterAntennaGain, const double receiverAntennaGain, const Coord transmissionPosition, const Coord receptionPosition) :
        radioMedium(radioMedium), transmitterAntennaGain(transmitterAntennaGain), receiverAntennaGain(receiverAntennaGain), transmissionPosition(transmissionPosition), receptionPosition(receptionPosition), distance(m(transmissionPosition.distance(receptionPosition)))
    {
    }

    virtual double getValue(const Point<simtime_t, Hz>& p) const override {
        Hz frequency = std::get<1>(p);
        auto propagationSpeed = radioMedium->getPropagation()->getPropagationSpeed();
        auto pathLoss = radioMedium->getPathLoss()->computePathLoss(propagationSpeed, frequency, distance);
        auto obstacleLoss = radioMedium->getObstacleLoss() ? radioMedium->getObstacleLoss()->computeObstacleLoss(frequency, transmissionPosition, receptionPosition) : 1;
        return std::min(1.0, transmitterAntennaGain * receiverAntennaGain * pathLoss * obstacleLoss);
    }

    virtual void partition(const Interval<simtime_t, Hz>& i, const std::function<void (const Interval<simtime_t, Hz>&, const IFunction<double, Domain<simtime_t, Hz>> *)> f) const override {
        throw cRuntimeError("Cannot partition");
    }

    virtual bool isFinite(const Interval<simtime_t, Hz>& i) const override { return true; }
};

/**
 * This mathematical function provides the transmission signal attenuation for any given
 * space, time, and frequency coordinates.
 */
class INET_API SpaceAndFrequencyAttenuationFunction : public FunctionBase<double, Domain<m, m, m, simtime_t, Hz>>
{
  protected:
    const Ptr<const IFunction<double, Domain<Quaternion>>> transmitterAntennaGainFunction;
    const Ptr<const IFunction<double, Domain<mps, m, Hz>>> pathLossFunction;
    const Ptr<const IFunction<double, Domain<m, m, m, m, m, m, Hz>>> obstacleLossFunction;
    const Point<m, m, m> startPosition;
    const Quaternion startOrientation;
    const mps propagationSpeed;

  public:
    SpaceAndFrequencyAttenuationFunction(const Ptr<const IFunction<double, Domain<Quaternion>>>& transmitterAntennaGainFunction, const Ptr<const IFunction<double, Domain<mps, m, Hz>>>& pathLossFunction, const Ptr<const IFunction<double, Domain<m, m, m, m, m, m, Hz>>>& obstacleLossFunction, const Point<m, m, m> startPosition, const Quaternion startOrientation, const mps propagationSpeed) :
        transmitterAntennaGainFunction(transmitterAntennaGainFunction), pathLossFunction(pathLossFunction), obstacleLossFunction(obstacleLossFunction), startPosition(startPosition), startOrientation(startOrientation), propagationSpeed(propagationSpeed) { }

    virtual double getValue(const Point<m, m, m, simtime_t, Hz>& p) const override {
        m x = std::get<0>(p);
        m y = std::get<1>(p);
        m z = std::get<2>(p);
        m startX = std::get<0>(startPosition);
        m startY = std::get<1>(startPosition);
        m startZ = std::get<2>(startPosition);
        m dx = x - startX;
        m dy = y - startY;
        m dz = z - startZ;
        Hz frequency = std::get<4>(p);
        m distance = m(sqrt(dx * dx + dy * dy + dz * dz));
        auto direction = Quaternion::rotationFromTo(Coord::X_AXIS, Coord(dx.get(), dy.get(), dz.get()));
        auto antennaLocalDirection = startOrientation.inverse() * direction;
        double transmitterAntennaGain = distance == m(0) ? 1 : transmitterAntennaGainFunction->getValue(Point<Quaternion>(antennaLocalDirection));
        double pathLoss = pathLossFunction->getValue(Point<mps, m, Hz>(propagationSpeed, distance, frequency));
        double obstacleLoss = obstacleLossFunction != nullptr ? obstacleLossFunction->getValue(Point<m, m, m, m, m, m, Hz>(startX, startY, startZ, x, y, z, frequency)) : 1;
        return std::min(1.0, transmitterAntennaGain * pathLoss * obstacleLoss);
    }

    virtual void partition(const Interval<m, m, m, simtime_t, Hz>& i, const std::function<void (const Interval<m, m, m, simtime_t, Hz>&, const IFunction<double, Domain<m, m, m, simtime_t, Hz>> *)> f) const override {
        const auto& lower = i.getLower();
        const auto& upper = i.getUpper();
        if (std::get<0>(lower) == std::get<0>(upper) && std::get<1>(lower) == std::get<1>(upper) && std::get<2>(lower) == std::get<2>(upper) && (i.getClosed() & 0b11100) == 0b11100)
            throw cRuntimeError("Cannot partition");
        else
            throw cRuntimeError("Invalid arguments");
    }

    virtual bool isFinite(const Interval<m, m, m, simtime_t, Hz>& i) const override { return true; }
};

class INET_API PropagatedTransmissionPowerFunction : public FunctionBase<WpHz, Domain<m, m, m, simtime_t, Hz>>
{
  protected:
    const Ptr<const IFunction<WpHz, Domain<simtime_t, Hz>>> transmissionPowerFunction;
    const Point<m, m, m> startPosition;
    const mps propagationSpeed;

  public:
    PropagatedTransmissionPowerFunction(const Ptr<const IFunction<WpHz, Domain<simtime_t, Hz>>>& transmissionPowerFunction, const Point<m, m, m>& startPosition, mps propagationSpeed) : transmissionPowerFunction(transmissionPowerFunction), startPosition(startPosition), propagationSpeed(propagationSpeed) { }

    virtual const Point<m, m, m>& getStartPosition() const { return startPosition; }

    virtual WpHz getValue(const Point<m, m, m, simtime_t, Hz>& p) const override {
        m x = std::get<0>(p);
        m y = std::get<1>(p);
        m z = std::get<2>(p);
        m startX = std::get<0>(startPosition);
        m startY = std::get<1>(startPosition);
        m startZ = std::get<2>(startPosition);
        m dx = x - startX;
        m dy = y - startY;
        m dz = z - startZ;
        simtime_t time = std::get<3>(p);
        Hz frequency = std::get<4>(p);
        m distance = m(sqrt(dx * dx + dy * dy + dz * dz));
        if (std::isinf(distance.get()))
            return WpHz(0);
        simtime_t propagationTime = s(distance / propagationSpeed).get();
        return transmissionPowerFunction->getValue(Point<simtime_t, Hz>(time - propagationTime, frequency));
    }

    virtual void partition(const Interval<m, m, m, simtime_t, Hz>& i, const std::function<void (const Interval<m, m, m, simtime_t, Hz>&, const IFunction<WpHz, Domain<m, m, m, simtime_t, Hz>> *)> f) const override {
        const auto& lower = i.getLower();
        const auto& upper = i.getUpper();
        if (std::get<0>(lower) == std::get<0>(upper) && std::get<1>(lower) == std::get<1>(upper) && std::get<2>(lower) == std::get<2>(upper) && (i.getClosed() & 0b11100) == 0b11100) {
            m x = std::get<0>(lower);
            m y = std::get<1>(lower);
            m z = std::get<2>(lower);
            m startX = std::get<0>(startPosition);
            m startY = std::get<1>(startPosition);
            m startZ = std::get<2>(startPosition);
            m dx = x - startX;
            m dy = y - startY;
            m dz = z - startZ;
            m distance = m(sqrt(dx * dx + dy * dy + dz * dz));
            simtime_t propagationTime = s(distance / propagationSpeed).get();
            Point<simtime_t, Hz> l1(std::get<3>(lower) - propagationTime, std::get<4>(i.getLower()));
            Point<simtime_t, Hz> u1(std::get<3>(upper) - propagationTime, std::get<4>(i.getUpper()));
            Interval<simtime_t, Hz> i1(l1, u1, i.getClosed() & 0b11);
            transmissionPowerFunction->partition(i1, [&] (const Interval<simtime_t, Hz>& i2, const IFunction<WpHz, Domain<simtime_t, Hz>> *g) {
                Interval<m, m, m, simtime_t, Hz> i3(
                    Point<m, m, m, simtime_t, Hz>(std::get<0>(lower), std::get<1>(lower), std::get<2>(lower), std::get<0>(i2.getLower()) + propagationTime, std::get<1>(i2.getLower())),
                    Point<m, m, m, simtime_t, Hz>(std::get<0>(upper), std::get<1>(upper), std::get<2>(upper), std::get<0>(i2.getUpper()) + propagationTime, std::get<1>(i2.getUpper())),
                    0b11100 | i2.getClosed());
                if (auto cg = dynamic_cast<const ConstantFunction<WpHz, Domain<simtime_t, Hz>> *>(g)) {
                    ConstantFunction<WpHz, Domain<m, m, m, simtime_t, Hz>> h(cg->getConstantValue());
                    f(i3, &h);
                }
                else if (auto lg = dynamic_cast<const LinearInterpolatedFunction<WpHz, Domain<simtime_t, Hz>> *>(g)) {
                    LinearInterpolatedFunction<WpHz, Domain<m, m, m, simtime_t, Hz>> h(i3.getLower(), i3.getUpper(), lg->getValue(i2.getLower()), lg->getValue(i2.getUpper()), lg->getDimension() + 3);
                    f(i3, &h);
                }
                else
                    throw cRuntimeError("TODO");
            });
        }
        else
            throw cRuntimeError("TODO");
    }
};

class INET_API PathLossFunction : public FunctionBase<double, Domain<mps, m, Hz>>
{
  protected:
    const IPathLoss *pathLoss;

  public:
    PathLossFunction(const IPathLoss *pathLoss) : pathLoss(pathLoss) { }

    virtual double getValue(const Point<mps, m, Hz>& p) const override {
        mps propagationSpeed = std::get<0>(p);
        Hz frequency = std::get<2>(p);
        m distance = std::get<1>(p);
        return pathLoss->computePathLoss(propagationSpeed, frequency, distance);
    }

    virtual void partition(const Interval<mps, m, Hz>& i, const std::function<void (const Interval<mps, m, Hz>&, const IFunction<double, Domain<mps, m, Hz>> *)> f) const override {
        throw cRuntimeError("TODO");
    }
};

class INET_API ObstacleLossFunction : public FunctionBase<double, Domain<m, m, m, m, m, m, Hz>>
{
  protected:
    const IObstacleLoss *obstacleLoss;

  public:
    ObstacleLossFunction(const IObstacleLoss *obstacleLoss) : obstacleLoss(obstacleLoss) { }

    virtual double getValue(const Point<m, m, m, m, m, m, Hz>& p) const override {
        Coord transmissionPosition(std::get<0>(p).get(), std::get<1>(p).get(), std::get<2>(p).get());
        Coord receptionPosition(std::get<3>(p).get(), std::get<4>(p).get(), std::get<5>(p).get());
        Hz frequency = std::get<6>(p);
        return obstacleLoss->computeObstacleLoss(frequency, transmissionPosition, receptionPosition);
    }

    virtual void partition(const Interval<m, m, m, m, m, m, Hz>& i, const std::function<void (const Interval<m, m, m, m, m, m, Hz>&, const IFunction<double, Domain<m, m, m, m, m, m, Hz>> *)> f) const override {
        throw cRuntimeError("TODO");
    }
};

class INET_API AntennaGainFunction : public IFunction<double, Domain<Quaternion>>
{
  protected:
    const IAntennaGain *antennaGain;

  public:
    AntennaGainFunction(const IAntennaGain *antennaGain) : antennaGain(antennaGain) { }

    virtual Interval<double> getRange() const override { throw cRuntimeError("TODO"); }
    virtual Interval<Quaternion> getDomain() const override { throw cRuntimeError("TODO"); }

    virtual double getValue(const Point<Quaternion>& p) const override {
        return antennaGain->computeGain(std::get<0>(p));
    }

    virtual void partition(const Interval<Quaternion>& i, const std::function<void (const Interval<Quaternion>&, const IFunction<double, Domain<Quaternion>> *)> f) const override { throw cRuntimeError("TODO"); }

    virtual bool isFinite() const override { throw cRuntimeError("TODO"); }
    virtual bool isFinite(const Interval<Quaternion>& i) const override { throw cRuntimeError("TODO"); }

    virtual double getMin() const override { throw cRuntimeError("TODO"); }
    virtual double getMin(const Interval<Quaternion>& i) const override { throw cRuntimeError("TODO"); }

    virtual double getMax() const override { throw cRuntimeError("TODO"); }
    virtual double getMax(const Interval<Quaternion>& i) const override { throw cRuntimeError("TODO"); }

    virtual double getMean() const override { throw cRuntimeError("TODO"); }
    virtual double getMean(const Interval<Quaternion>& i) const override { throw cRuntimeError("TODO"); }

    virtual double getIntegral() const override { throw cRuntimeError("TODO"); }
    virtual double getIntegral(const Interval<Quaternion>& i) const override { throw cRuntimeError("TODO"); }

    virtual const Ptr<const IFunction<double, Domain<Quaternion>>> add(const Ptr<const IFunction<double, Domain<Quaternion>>>& o) const override { throw cRuntimeError("TODO"); }
    virtual const Ptr<const IFunction<double, Domain<Quaternion>>> subtract(const Ptr<const IFunction<double, Domain<Quaternion>>>& o) const override { throw cRuntimeError("TODO"); }
    virtual const Ptr<const IFunction<double, Domain<Quaternion>>> multiply(const Ptr<const IFunction<double, Domain<Quaternion>>>& o) const override { throw cRuntimeError("TODO"); }
    virtual const Ptr<const IFunction<double, Domain<Quaternion>>> divide(const Ptr<const IFunction<double, Domain<Quaternion>>>& o) const override { throw cRuntimeError("TODO"); }
};

} // namespace physicallayer

} // namespace inet

#endif // #ifndef __INET_POWERFUNCTION_H_

