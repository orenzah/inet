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

#ifndef __INET_OPENSTREETMAP_H
#define __INET_OPENSTREETMAP_H

#include "inet/common/INETDefs.h"

namespace inet {

namespace osm {

//TODO wrap into getters

class Node {
public:
  std::string id;
  double lat, lon;
  std::map<std::string,std::string> tags;  //TODO stringpool; char*[]: k1,v1,k2,v2,...,nullptr
  std::string getTag(const std::string& k) const {auto it = tags.find(k); return it == tags.end() ? "" : it->second;}
};

class Way {
public:
  std::string id;
  std::vector<Node*> nodes;
  std::map<std::string,std::string> tags;
  std::string getTag(const std::string& k) const {auto it = tags.find(k); return it == tags.end() ? "" : it->second;}
};

//TODO
//class Relationship {
//};

class Map {
public:
  double minlat, minlon, maxlat, maxlon;
  std::vector<Node*> nodes;
  std::vector<Way*> ways;
  //TODO relationships
  ~Map();
  static Map loadMap(cXMLElement *mapRoot);
};

} // namespace osm

} // namespace inet

#endif // ifndef __INET_OPENSTREETMAP_H
