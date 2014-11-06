#include "Pathing.h"
#include <vector>
#include <ostream>
#include <iostream>

namespace Pathing {

std::ostream& operator<<(std::ostream& out, Grid<short>::Node& node) {
    out << "(" << node.x << ", " << node.y << ")";
    return out;
}

JumpPointSearch::JumpPointSearch(HeuristicFunction heuristic, int search_radius) 
    : m_Heuristic(heuristic), m_Grid(nullptr), m_Goal(nullptr), m_Start(nullptr), m_SearchRadius(search_radius) { }

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

    if (std::abs(nx - m_Start->x) > m_SearchRadius || std::abs(nx - m_Start->y > m_SearchRadius)) return nullptr;

    if (dx != 0 && dy != 0) {
        // Check diagonal forced neighbors
        if ((m_Grid->IsOpen(nx - dx, ny + dy) && !m_Grid->IsOpen(nx - dx, ny)) ||
            (m_Grid->IsOpen(nx + dx, ny - dy) && !m_Grid->IsOpen(nx, ny - dy)))
        {
            return m_Grid->GetNode(nx, ny);
        }

        // Expand horizontally and vertically
        if (Jump(nx + dx, ny, nx, ny) ||Jump(nx, ny + dy, nx, ny))
            return m_Grid->GetNode(nx, ny);
    } else {
        if (dx != 0) {
            // Check horizontal forced neighbors
            if ((m_Grid->IsOpen(nx + dx, ny + 1) && !m_Grid->IsOpen(nx, ny + 1)) ||
                (m_Grid->IsOpen(nx + dx, ny - 1) && !m_Grid->IsOpen(nx, ny - 1)))
            {
                return m_Grid->GetNode(nx, ny);
            }
        } else {
            // Check vertical forced neighbors
            if ((m_Grid->IsOpen(nx + 1, ny + dy) && !m_Grid->IsOpen(nx + 1, ny)) ||
                (m_Grid->IsOpen(nx - 1, ny + dy) && !m_Grid->IsOpen(nx - 1, ny)))
            {
                return m_Grid->GetNode(nx, ny);
            }
        }
    }

    if (m_Grid->IsOpen(nx + dx, ny) || m_Grid->IsOpen(nx, ny + dy))
        return Jump(nx + dx, ny + dy, nx, ny);
    return nullptr;
}

std::vector<JPSNode*> JumpPointSearch::FindNeighbors(Node* node) {
    // no pruning if no parent
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
    std::vector<Node*> neighbors = FindNeighbors(node);

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
    m_Start = start;

    if (!start || !m_Goal) return std::vector<Node*>();

    start->opened = true;

    m_OpenSet.Push(start);

    while (!m_OpenSet.Empty()) {
        Node* current = m_OpenSet.Pop();

        current->closed = true;

        if (NearGoal(current, m_Goal))
            return Backtrace(current);

        IdentifySuccessors(current);
    }

    return std::vector<Node*>();
}

} // ns