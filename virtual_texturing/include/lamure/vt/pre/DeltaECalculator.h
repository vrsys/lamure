//
// Created by sebastian on 24.02.18.
//

#ifndef TILE_PROVIDER_DELTAECALCULATOR_H
#define TILE_PROVIDER_DELTAECALCULATOR_H

#define DELTA_E_CALCULATOR_LOG_PROGRESS

#include <chrono>
#include <iomanip>
#include <iostream>
#include <lamure/vt/pre/AtlasFile.h>
#include <lamure/vt/pre/OffsetIndex.h>

namespace vt {
    namespace pre {
        class DeltaECalculator : public AtlasFile {
        public:
            explicit DeltaECalculator(const char *fileName);
            void calculate(size_t maxMemory);
        };
    }
}

#endif //TILE_PROVIDER_DELTAECALCULATOR_H
