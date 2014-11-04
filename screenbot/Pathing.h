#ifndef PATHING_H_
#define PATHING_H_

#include <queue>
#include <vector>
#include <iosfwd>
#include <functional>

namespace Pathing {

template <typename IntType = int>
class Grid {
public:
    struct Node {
        IntType x;
        IntType y;
        bool solid;
        Node* parent;
        bool opened;
        bool closed;
        bool near_wall;
        int g;
        int h;
    };
private:
    Node* m_Nodes;
    IntType m_Width;
    IntType m_Height;

    Node* GetNode(IntType x, IntType y) const {
        return &m_Nodes[y * m_Height + x];
    }

public:
    Grid(int width, int height) : m_Nodes(nullptr), m_Width(width), m_Height(height) {
        m_Nodes = new Node[width * height];

        for (IntType y = 0; y < height; ++y) {
            for (IntType x = 0; x < width; ++x) {
                m_Nodes[y * height + x].x = x;
                m_Nodes[y * height + x].y = y;
                m_Nodes[y * height + x].near_wall = false;
                m_Nodes[y * height + x].solid = false;
            }
        }
    }

    ~Grid() {
        if (m_Nodes) delete m_Nodes;
    }

    Grid(const Grid& other) {
        m_Nodes = new Node[width * height];
        memcpy(m_Nodes, other.m_Nodes, sizeof(Node) * m_Width * m_Height);
    }

    Grid(Grid&& other) {
        m_Nodes = std::move(other.m_Nodes);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        other.m_Nodes = NULL;
    }

    IntType GetWidth() const { return m_Width; }
    IntType GetHeight() const { return m_Height; }

    Node* GetNode(IntType x, IntType y) {
        return &m_Nodes[y * m_Height + x];
    }

    bool IsSolid(IntType x, IntType y) const {
        return GetNode(x, y)->solid;
    }

    bool IsValid(IntType x, IntType y) const {
        return x >= 0 && x < m_Width && y >= 0 && y < m_Height;
    }

    bool IsOpen(IntType x, IntType y) const {
        Node* node = GetNode(x, y);
        return IsValid(x, y) && !IsSolid(x, y) && !GetNode(x, y)->near_wall;
    }

    void SetSolid(IntType x, IntType y, bool solid) {
        static std::vector<Grid<short>::Node> directions = {
                { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 },
                { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
        };

        GetNode(x, y)->solid = solid;

        if (solid) {
            for (auto& dir : directions) {
                for (int i = 0; i < 1; ++i) {
                    if (IsValid(x + dir.x + i, y + dir.y + i))
                        GetNode(x + dir.x + i, y + dir.y + i)->near_wall = true;
                }
            }
        }
    }

    std::vector<Node*> GetNeighbors(IntType x, IntType y) {
        std::vector<Node*> neighbors;
        static std::vector<Node> directions = {
                { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 },
                { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 }
        };

        neighbors.reserve(8);

        for (Node& dir : directions) {
            if (IsOpen(x + dir.x, y + dir.y))
                neighbors.push_back(GetNode(x + dir.x, y + dir.y));
        }

        return neighbors;
    }

    std::vector<Node*> GetNeighbors(Node* node) {
        return GetNeighbors(node->x, node->y);
    }
};

std::ostream& operator<<(std::ostream& out, Grid<short>::Node& node);

namespace Heuristic {

template <typename IntType>
int Manhattan(typename Grid<IntType>::Node* a, typename Grid<IntType>::Node* b) {
    return std::abs(a->x - b->x) + std::abs(a->y - b->y);
}

template <typename IntType>
int Chebyshev(typename Grid<IntType>::Node* a, typename Grid<IntType>::Node* b) {
    int dx = a->x - b->x;
    int dy = a->y - b->y;
    return std::max(dx, dy);
}


} // ns

template <typename T, typename Compare, typename Container = std::deque<T>>
class PriorityQueue {
private:
    Container m_Container;
    Compare m_Comp;

public:
    void Push(T item) {
        m_Container.push_back(item);
        std::push_heap(m_Container.begin(), m_Container.end(), m_Comp);
    }

    T Pop() {
        T item = m_Container.front();
        std::pop_heap(m_Container.begin(), m_Container.end(), m_Comp);
        m_Container.pop_back();
        return item;
    }

    void Update() {
        std::make_heap(m_Container.begin(), m_Container.end(), m_Comp);
    }

    bool Empty() const { return m_Container.empty(); }
};

class JumpPointSearch {
public:
    typedef short IntType;
    typedef Grid<IntType>::Node Node;
    typedef std::function<int(Grid<IntType>::Node* first, Grid<IntType>::Node* second)> HeuristicFunction;

private:
    struct JPSCompare {
        bool operator()(Node* lhs, Node* rhs) const {
            return lhs->g + lhs->h > rhs->g + rhs->h;
        }
    };

    HeuristicFunction m_Heuristic;
    Grid<IntType>* m_Grid;
    PriorityQueue<Node*, JPSCompare> m_OpenSet;
    Node* m_Goal;

    Node* Jump(IntType nx, IntType ny, IntType cx, IntType cy);
    void IdentifySuccessors(Node* node);
    void ResetGrid();
    std::vector<Node*> Backtrace(Node* node);

public:
    JumpPointSearch(HeuristicFunction heuristic);
    std::vector<Node*> operator()(IntType startX, IntType startY, IntType endX, IntType endY, Grid<IntType>& grid);
};

typedef JumpPointSearch::Node JPSNode;
typedef std::vector<JPSNode*> Plan;

} // ns

#endif
