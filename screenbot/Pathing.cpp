#include "Pathing.h"
#include <vector>
#include <ostream>

namespace Pathing {

std::ostream& operator<<(std::ostream& out, Grid<short>::Node& node) {
    out << "(" << node.x << ", " << node.y << ")";
    return out;
}

JumpPointSearch::JumpPointSearch(HeuristicFunction heuristic) 
    : m_Heuristic(heuristic), m_Grid(nullptr), m_Goal(nullptr) { }

bool JumpPointSearch::NearGoal(Node* a, Node* b) {
    int dx = a->x - b->x;
    int dy = a->y - b->y;
    return std::sqrt(dx * dx + dy * dy) <= 1;
}

JumpPointSearch::Node* JumpPointSearch::Jump(IntType nx, IntType ny, IntType cx, IntType cy) {
    int dx = nx - cx;
    int dy = ny - cy;

    if (NearGoal(m_Grid->GetNode(nx, ny), m_Goal)) return m_Grid->GetNode(nx, ny);
    if (!m_Grid->IsOpen(nx, ny)) return nullptr;

    int offsetX = nx;
    int offsetY = ny;

    if (dx != 0 && dy != 0) {
        while (true) {
            // Check diagonal forced neighbors
            if ((m_Grid->IsOpen(offsetX - dx, offsetY + dy) && !m_Grid->IsOpen(offsetX - dx, offsetY)) ||
                (m_Grid->IsOpen(offsetX + dx, offsetY - dy) && !m_Grid->IsOpen(offsetX, offsetY - dy)))
            {
                return m_Grid->GetNode(offsetX, offsetY);
            }

            // Expand horizontally and vertically
            Node* jx = Jump(offsetX + dx, offsetY, offsetX, offsetY);
            Node *jy = Jump(offsetX, offsetY + dy, offsetX, offsetY);

            if (jx || jy)
                return m_Grid->GetNode(offsetX, offsetY);

            offsetX += dx;
            offsetY += dy;

            if (NearGoal(m_Grid->GetNode(offsetX, offsetY), m_Goal)) return m_Grid->GetNode(offsetX, offsetY);
            if (!m_Grid->IsOpen(offsetX, offsetY)) return nullptr;
        }
    } else {
        if (dx != 0) {
            // Check horizontal forced neighbors
            while (true) {
                if ((m_Grid->IsOpen(offsetX + dx, ny + 1) && !m_Grid->IsOpen(offsetX, ny + 1)) ||
                    (m_Grid->IsOpen(offsetX + dx, ny - 1) && !m_Grid->IsOpen(offsetX, ny - 1)))
                {
                    return m_Grid->GetNode(offsetX, ny);
                }

                offsetX += dx;

                if (NearGoal(m_Grid->GetNode(offsetX, ny), m_Goal)) return m_Grid->GetNode(offsetX, ny);
                if (!m_Grid->IsOpen(offsetX, ny)) return nullptr;
            }
        } else {
            // Check vertical forced neighbors
            while (true) {
                if ((m_Grid->IsOpen(nx + 1, offsetY + dy) && !m_Grid->IsOpen(nx + 1, offsetY)) ||
                    (m_Grid->IsOpen(nx - 1, offsetY + dy) && !m_Grid->IsOpen(nx - 1, offsetY)))
                {
                    return m_Grid->GetNode(nx, offsetY);
                }

                offsetY += dy;

                if (NearGoal(m_Grid->GetNode(nx, offsetY), m_Goal)) return m_Grid->GetNode(nx, offsetY);
                if (!m_Grid->IsOpen(nx, offsetY)) return nullptr;
            }
        }
    }

    return nullptr;
}

void JumpPointSearch::IdentifySuccessors(Node* node) {
    std::vector<Node*> neighbors = m_Grid->GetNeighbors(node);

    int ng = 0;

    for (Node* neighbor : neighbors) {
        Node* jump_point = Jump(neighbor->x, neighbor->y, node->x, node->y);

        if (jump_point) {
            if (jump_point->closed) continue;

            int dx = jump_point->x - node->x;
            int dy = jump_point->y - node->y;

            int dist = static_cast<int>(std::sqrt(dx * dx + dy * dy));
            ng = node->g + dist;

            if (!jump_point->opened || ng < jump_point->g) {
                jump_point->parent = node;
                jump_point->g = ng;

                jump_point->h = m_Heuristic(jump_point, m_Goal);

                if (!jump_point->opened) {
                    jump_point->opened = true;
                    m_OpenSet.Push(jump_point);
                } else {
                    m_OpenSet.Update();
                }
            }
        }
    }
}

void JumpPointSearch::ResetGrid() {
    int width = m_Grid->GetWidth();
    int height = m_Grid->GetHeight();

    for (IntType y = 0; y < height; ++y) {
        for (IntType x = 0; x < width; ++x) {
            m_Grid->GetNode(x, y)->closed = false;
            m_Grid->GetNode(x, y)->opened = false;
            m_Grid->GetNode(x, y)->parent = nullptr;
            m_Grid->GetNode(x, y)->g = 0;
            m_Grid->GetNode(x, y)->h = 0;
        }
    }
}

std::vector<JumpPointSearch::Node*> JumpPointSearch::Backtrace(Node* node) {
    std::vector<Node*> path;

    Node* current = node;

    while (current) {
        path.push_back(current);
        current = current->parent;
    }

    return path;
}

std::vector<JumpPointSearch::Node*> JumpPointSearch::operator()(IntType startX, IntType startY, IntType endX, IntType endY, Grid<IntType>& grid) {
    m_Grid = &grid;
    m_Goal = m_Grid->GetNode(endX, endY);
    ResetGrid();

    Node* start = m_Grid->GetNode(startX, startY);
    Node* end = m_Grid->GetNode(endX, endY);

    start->opened = true;

    m_OpenSet.Push(start);

    while (!m_OpenSet.Empty()) {
        Node* current = m_OpenSet.Pop();

        current->closed = true;

        if (NearGoal(current, end))
            return Backtrace(current);

        IdentifySuccessors(current);
    }

    return std::vector<Node*>();
}

} // ns