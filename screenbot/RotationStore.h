#ifndef ROTATION_STORE_H_
#define ROTATION_STORE_H_

#include "Config.h"

namespace Ships {
    class RotationStore {
    private:
        u64 m_Rotations[8][40];
        
        void LoadDefaults();
    public:
        RotationStore(Config& config);

        int GetRotation(u64 pixval);
    };
}

#endif
