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

#include "OpenStreetMap.h"

namespace inet {

namespace osm {

Map Map::loadMap(cXMLElement *mapRoot)
{
    Map map;
    std::map<std::string,Node*> nodeById;

    cXMLElement *boundsElement = mapRoot->getFirstChildWithTag("bounds");
    map.minlat = map.minlon = map.maxlat = map.maxlon = NAN;

    if (boundsElement) {
        map.minlat = std::strtod(boundsElement->getAttribute("minlat"), nullptr);
        map.minlon = std::strtod(boundsElement->getAttribute("minlon"), nullptr);
        map.maxlat = std::strtod(boundsElement->getAttribute("maxlat"), nullptr);
        map.maxlon = std::strtod(boundsElement->getAttribute("maxlon"), nullptr);
    }

    for (cXMLElement *child : mapRoot->getChildrenByTagName("node")) {
        Node *node = new Node();
        node->id = child->getAttribute("id");
        node->lat = std::strtod(child->getAttribute("lat"), nullptr);
        node->lon = std::strtod(child->getAttribute("lon"), nullptr); //TODO error

        for (cXMLElement *nodeChild : child->getChildren()) {
            if (strcmp(nodeChild->getTagName(),"tag")==0) {
                std::string k = nodeChild->getAttribute("k");
                std::string v = nodeChild->getAttribute("v");
                node->tags[k] = v;
            }
        }
        map.nodes.push_back(node);
        nodeById[node->id] = node;
    }

    for (cXMLElement *child : mapRoot->getChildrenByTagName("way")) {
        Way *way = new Way();
        way->id = child->getAttribute("id");
        for (cXMLElement *wayChild : child->getChildren()) {
            if (strcmp(wayChild->getTagName(),"nd")==0) {
                std::string ref = wayChild->getAttribute("ref");
                Node *node = nodeById[ref]; //TODO check
                way->nodes.push_back(node);
            }
            if (strcmp(wayChild->getTagName(),"tag")==0) {
                std::string k = wayChild->getAttribute("k");
                std::string v = wayChild->getAttribute("v");
                way->tags[k] = v;
            }
        }
        map.ways.push_back(way);
    }

    return map;
}

} // namespace osm

} // namespace inet

