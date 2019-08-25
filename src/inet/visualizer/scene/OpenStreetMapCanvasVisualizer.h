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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_OPENSTREETMAPCANVASVISUALIZER_H
#define __INET_OPENSTREETMAPCANVASVISUALIZER_H

#include "inet/common/geometry/common/CanvasProjection.h"
#include "inet/common/geometry/common/GeographicCoordinateSystem.h"
#include "inet/visualizer/base/SceneVisualizerBase.h"
#include "inet/visualizer/scene/OpenStreetMap.h"

namespace inet {

namespace visualizer {

class INET_API OpenStreetMapCanvasVisualizer : public SceneVisualizerBase
{
  protected:
    double zIndex = NaN;
    cCanvas *canvas = nullptr;
    IGeographicCoordinateSystem *coordinateSystem = nullptr;
    CanvasProjection *canvasProjection = nullptr;

  protected:
    virtual void initialize(int stage) override;
    cFigure::Point toCanvas(const osm::Map& map, double lat, double lon);
    void drawMap(const osm::Map& map);
};

} // namespace visualizer

} // namespace inet

#endif // ifndef __INET_OPENSTREETMAPCANVASVISUALIZER_H

