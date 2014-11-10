#include "Pathing.h"

#include <vector>
#include <ostream>
#include <iostream>

namespace Pathing {

std::ostream& operator<<(std::ostream& out, Grid<short>::Node& node) {
    out << "(" << node.x << ", " << node.y << ")";
    return out;
}

JumpPointSearch::JumpPointSearch(HeuristicFunction heuristic) 
    : m_Heuristic(heuristic), m_Grid(nullptr), m_Goal(nullptr), m_Start(nullptr) { }

bool JumpPointSearch::NearGoal(Node* a, Node* b) {
    int dx = a->x - b->x;
    int dy = a->y - b->y;
    return std::sqrt(dx * dx + dy * dy) <= 1;
}

JumpPointSearch::Node* JumpPointSearch::Jump(IntType nx, IntType ny, IntType cx, IntType cy) {
    auto clamp = [](int a, int min, int max) {
        if (a < min) return min;
        if (a > max) return max;
        return a;
    };
    int dx = clamp(nx - cx, -1, 1);
    int dy = clamp(ny - cy, -1, 1);

    if (m_Grid->IsValid(nx, ny) && NearGoal(m_Grid->GetNode(nx, ny), m_Goal)) return m_Grid->GetNode(nx, ny);
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
            if (Jump(offsetX + dx, offsetY, offsetX, offsetY) || Jump(offsetX, offsetY + dy, offsetX, offsetY))
                return m_Grid->GetNode(offsetX, offsetY);

            offsetX += dx;
            offsetY += dy;

            if (m_Grid->IsValid(offsetX, offsetY) && NearGoal(m_Grid->GetNode(offsetX, offsetY), m_Goal)) return m_Grid->GetNode(offsetX, offsetY);
            if (!m_Grid->IsOpen(offsetX, offsetY)) return nullptr;
        }
    } else {
        if (dx != 0) {
            while (true) {
                // Check horizontal forced neighbors
                if ((m_Grid->IsOpen(offsetX + dx, offsetY + 1) && !m_Grid->IsOpen(offsetX, offsetY + 1)) ||
                    (m_Grid->IsOpen(offsetX + dx, offsetY - 1) && !m_Grid->IsOpen(offsetX, offsetY - 1)))
                {
                    return m_Grid->GetNode(offsetX, offsetY);
                }

                offsetX += dx;

                if (m_Grid->IsValid(offsetX, offsetY) && NearGoal(m_Grid->GetNode(offsetX, offsetY), m_Goal)) return m_Grid->GetNode(offsetX, offsetY);
                if (!m_Grid->IsOpen(offsetX, offsetY)) return nullptr;
            }
        } else {
            while (true) {
                // Check vertical forced neighbors
                if ((m_Grid->IsOpen(offsetX + 1, offsetY + dy) && !m_Grid->IsOpen(offsetX + 1, offsetY)) ||
                    (m_Grid->IsOpen(offsetX - 1, offsetY + dy) && !m_Grid->IsOpen(offsetX - 1, offsetY)))
                {
                    return m_Grid->GetNode(offsetX, offsetY);
                }

                offsetY += dy;

                if (m_Grid->IsValid(offsetX, offsetY) && NearGoal(m_Grid->GetNode(offsetX, offsetY), m_Goal)) return m_Grid->GetNode(offsetX, offsetY);
                if (!m_Grid->IsOpen(offsetX, offsetY)) return nullptr;
            }
        }
    }
    return nullptr;
}

std::vector<JPSNode*> JumpPointSearch::FindNeighbors(Node* node) {
    // no pruning if no parent
    if (!node) return std::vector<JPSNode*>();
    if (!node->parent) return m_Grid->GetNeighbors(node);

    std::vector<JPSNode*> neighbors;
    neighbors.reserve(8);

    int x = node->x;
    int y = node->y;
    int px = node->parent->x;
    int py = node->parent->y;

    int dx = (x - px) / std::max(std::abs(x - px), 1);
    int dy = (y - py) / std::max(std::abs(y - py), 1);

    if (dx != 0 && dy != 0) {
        // Search diagonally
        if (m_Grid->IsOpen(x, y + dy))
            neighbors.push_back(m_Grid->GetNode(x, y + dy));
        if (m_Grid->IsOpen(x + dx, y))
            neighbors.push_back(m_Grid->GetNode(x + dx, y));

        if (m_Grid->IsOpen(x, y + dy) || m_Grid->IsOpen(x + dx, y))
            neighbors.push_back(m_Grid->GetNode(x + dx, y + dy));

        if (!m_Grid->IsOpen(x - dx, y) && m_Grid->IsOpen(x, y + dy))
            neighbors.push_back(m_Grid->GetNode(x - dx, y + dy));

        if (!m_Grid->IsOpen(x, y - dy) && m_Grid->IsOpen(x + dx, y))
            neighbors.push_back(m_Grid->GetNode(x + dx, y - dy));
    } else {
        // Search horizontally/vertically
        if (dx == 0) {
            if (m_Grid->IsOpen(x, y + dy)) {
                neighbors.push_back(m_Grid->GetNode(x, y + dy));
                if (!m_Grid->IsOpen(x + 1, y))
                    neighbors.push_back(m_Grid->GetNode(x + 1, y + dy));
                if (!m_Grid->IsOpen(x - 1, y))
                    neighbors.push_back(m_Grid->GetNode(x - 1, y + dy));
            }
        } else {
            if (m_Grid->IsOpen(x + dx, y)) {
                neighbors.push_back(m_Grid->GetNode(x + dx, y));
                if (!m_Grid->IsOpen(x, y + 1))
                    neighbors.push_back(m_Grid->GetNode(x + dx, y + 1));
                if (!m_Grid->IsOpen(x, y - 1))
                    neighbors.push_back(m_Grid->GetNode(x + dx, y - 1));
            }
        }
    }
    return neighbors;
}

void JumpPointSearch::IdentifySuccessors(Node* node) {
    if (!node) return;

    std::vector<Node*> neighbors = FindNeighbors(node);

    int ng = 0;

    for (Node* neighbor : neighbors) {
        if (!neighbor) continue;
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
    m_Start = start;

    if (!start || !m_Goal) return std::vector<Node*>();

    start->opened = true;

    m_OpenSet.Push(start);

    while (!m_OpenSet.Empty()) {
        Node* current = m_OpenSet.Pop();

        if (!current) continue;
        current->closed = true;

        if (NearGoal(current, m_Goal))
            return Backtrace(current);

        IdentifySuccessors(current);
    }

    return std::vector<Node*>();
}

} // ns