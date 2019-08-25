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

#include "inet/common/ModuleAccess.h"
#include "OpenStreetMapCanvasVisualizer.h"

using namespace inet::osm;

namespace inet {

namespace visualizer {


Define_Module(OpenStreetMapCanvasVisualizer);

void OpenStreetMapCanvasVisualizer::initialize(int stage)
{
    SceneVisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LOCAL) {
        zIndex = par("zIndex");
        canvas = visualizationTargetModule->getCanvas();
        coordinateSystem = getModuleFromPar<IGeographicCoordinateSystem>(par("coordinateSystemModule"), this);
        canvasProjection = CanvasProjection::getCanvasProjection(canvas);

        cXMLElement *mapXml = par("mapFile").xmlValue();
        Map map = Map::loadMap(mapXml);
        EV << "loaded " << map.nodes.size() << " nodes, " << map.ways.size() << " ways\n";

        drawMap(map);
    }
}

cFigure::Point OpenStreetMapCanvasVisualizer::toCanvas(const Map& map, double lat, double lon)
{
    Coord coord = coordinateSystem->computeSceneCoordinate(GeoCoord(deg(lat), deg(lon), m(0)));
    cFigure::Point p = canvasProjection->computeCanvasPoint(coord);
    return p;
}

void OpenStreetMapCanvasVisualizer::drawMap(const Map& map)
{
    const cFigure::Color COLOR_HIGHWAY_PRIMARY = {255,255,120};
    const cFigure::Color COLOR_HIGHWAY_RESIDENTIAL = {240,240,240};
    const cFigure::Color COLOR_HIGHWAY_PATH = {128,128,128};

    for (const auto& way : map.ways) {
        std::vector<cFigure::Point> points;
        for (const auto& node : way->nodes)
            points.push_back(toCanvas(map, node->lat, node->lon));
        bool isArea = way->nodes.front() == way->nodes.back();

        //TODO z-order (primary road on top of others)
        if (!isArea) {
            // road, street, footway, etc.
            cPolylineFigure *polyline = new cPolylineFigure();
            polyline->setPoints(points);
            polyline->setZoomLineWidth(true);

            polyline->setName(way->id.c_str());
            std::string name = way->getTag("name");
            if (name != "")
                polyline->setTooltip(name.c_str());

            std::string highwayType = way->getTag("highway");
            if (highwayType == "primary" || highwayType == "secondary" || highwayType == "tertiary" ||
                highwayType == "primary_link" || highwayType == "secondary_link" || highwayType == "tertiary_link") {
                polyline->setLineWidth(8);
                polyline->setLineColor(COLOR_HIGHWAY_PRIMARY);
                polyline->setCapStyle(cFigure::CAP_ROUND);
                polyline->setJoinStyle(cFigure::JOIN_ROUND);
            }
            else if (highwayType == "residential" || highwayType == "service") {
                polyline->setLineWidth(4);
                polyline->setLineColor(COLOR_HIGHWAY_RESIDENTIAL);
                polyline->setCapStyle(cFigure::CAP_ROUND);
                polyline->setJoinStyle(cFigure::JOIN_ROUND);
            }
            else if (highwayType != "") { // footpath or similar
                polyline->setLineStyle(cFigure::LINE_DOTTED);
                polyline->setLineColor(COLOR_HIGHWAY_PATH);
            }
            else { // administrative boundary or some other non-path thing
                delete polyline;
                polyline = nullptr;
            }

            if (polyline)
                canvas->addFigure(polyline);
        }
        else {
            // building, park, etc.
            cPolygonFigure *polygon = new cPolygonFigure();
            points.pop_back();
            polygon->setPoints(points);

            polygon->setName(way->id.c_str());
            std::string name = way->getTag("name");
            if (name != "")
                polygon->setTooltip(name.c_str());

            polygon->setFilled(true);
            polygon->setFillOpacity(0.1);
            polygon->setLineOpacity(0.5);
            polygon->setLineColor(cFigure::GREY);
            if (way->getTag("building") != "")
                polygon->setFillColor(cFigure::RED);
            else
                polygon->setFillColor(cFigure::GREEN);
            canvas->addFigure(polygon);
        }
    }
}

} // namespace visualizer

} // namespace inet

