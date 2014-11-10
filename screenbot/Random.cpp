#include "Random.h"

#include <random>

namespace Random {

    std::random_device dev;
    std::mt19937 gen(dev());

    unsigned int GetU32(unsigned int start, unsigned int end) {
        std::uniform_int_distribution<unsigned int> dist(start, end);

        return dist(gen);
    }
}