#include "Pathing.h"
#include <queue>
#include <vector>
#include "Level.h"

namespace Path {

struct Node {
    int x;
    int y;
    int priority;

    Node(int x, int y, int priority) : x(x), y(y), priority(priority) { }

    bool operator==(const Node& rhs) {
        return x == rhs.x && y == rhs.y;
    }
};

struct NodeCompare {
    bool operator()(const Node& lhs, const Node& rhs) const {
        return lhs.priority > rhs.priority;
    }
};

bool operator<(const Node& lhs, const Node& rhs) {
    return lhs.priority < rhs.priority;
}

std::vector<Coord> GetNeighbors(Coord pos) {
    const int width = 1024;
    std::vector<Coord> neighbors;
    neighbors.resize(8);
    int x, y;

    for (int i = 0; i < 4; i++)
        neighbors[i] = Coord(0, 0);

    x = pos.x;
    y = pos.y - 1;
    if (x > 0 && x < width && y > 0 && y < width)
        neighbors[0] = Coord(x, y);

    x = pos.x + 1;
    y = pos.y;
    if (x > 0 && x < width && y > 0 && y < width)
        neighbors[1] = Coord(x, y);

    x = pos.x;
    y = pos.y + 1;
    if (x > 0 && x < width && y > 0 && y < width)
        neighbors[2] = Coord(x, y);

    x = pos.x - 1;
    y = pos.y;
    if (x > 0 && x < width && y > 0 && y < width)
        neighbors[3] = Coord(x, y);

    x = pos.x - 1;
    y = pos.y + 1;
    if (x > 0 && x < width && y > 0 && y < width)
        neighbors[4] = Coord(x, y);

    x = pos.x + 1;
    y = pos.y + 1;
    if (x > 0 && x < width && y > 0 && y < width)
        neighbors[5] = Coord(x, y);

    x = pos.x - 1;
    y = pos.y - 1;
    if (x > 0 && x < width && y > 0 && y < width)
        neighbors[6] = Coord(x, y);

    x = pos.x + 1;
    y = pos.y - 1;
    if (x > 0 && x < width && y > 0 && y < width)
        neighbors[7] = Coord(x, y);

    return neighbors;
}

Graph::Graph(const Level& level) : m_Level(level) {

}

std::vector<Coord> Graph::GetPath(const Coord& start, const Coord& goal) {
    std::priority_queue<Node, std::vector<Node>, NodeCompare> frontier;

    auto heuristic = [](const Coord& a, const Coord& b) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    };

    m_Graph.clear();

    frontier.emplace(start.x, start.y, 0);
    m_Graph[start] = Coord(0, 0);

    while (!frontier.empty()) {
        Node current_node = frontier.top();
        Coord current(current_node.x, current_node.y);
        frontier.pop();

        if (current == goal) break;

        std::vector<Coord> neighbors = GetNeighbors(Coord(current.x, current.y));
        for (Coord& next : neighbors) {
            if (!m_Level.IsSolid(next.x, next.y) && // middle
                !m_Level.IsSolid(next.x + 1, next.y) && // right
                !m_Level.IsSolid(next.x, next.y - 1) && // upper
                !m_Level.IsSolid(next.x, next.y - 1) && // lower
                !m_Level.IsSolid(next.x - 1, next.y - 1) && // upper-left
                !m_Level.IsSolid(next.x + 1, next.y - 1) && // upper-right
                !m_Level.IsSolid(next.x + 1, next.y + 1) && // lower-right
                !m_Level.IsSolid(next.x - 1, next.y + 1)) // lower-left
            {
                if (m_Graph.find(next) == m_Graph.end()) {
                    int priority = heuristic(goal, next);
                    frontier.emplace(next.x, next.y, priority);
                    m_Graph[next] = current;
                }
            }
        }
    }

    std::vector<Coord> path;
    Coord path_goal(goal);

    path.push_back(start);
    while (path_goal != start) {
        if (path_goal == Coord(0, 0)) break;
        path_goal = m_Graph[path_goal];
        path.push_back(path_goal);
    }

    return path;
}

}