#ifndef PATHING_H_
#define PATHING_H_

#include <map>
#include <vector>
#include "Common.h"

class Level;

namespace Path {

class Graph {
private:
    std::map<Coord, Coord> m_Graph;
    const Level& m_Level;

public:
    Graph(const Level& level);

    std::vector<Coord> GetPath(const Coord& start, const Coord& goal);
};

} // namespace

#endif
