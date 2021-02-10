/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VERIFYPN_STUBBORNTARJANMODELCHECKER_H
#define VERIFYPN_STUBBORNTARJANMODELCHECKER_H

#include "LTL/Stubborn/LTLStubbornSet.h"
#include "LTL/Algorithm/ModelChecker.h"

namespace LTL {
    class StubbornTarjanModelChecker : public ModelChecker<PetriEngine::ReducingSuccessorGenerator> {
    public:
        StubbornTarjanModelChecker(const PetriEngine::PetriNet &net, const Condition_ptr &query)
                : ModelChecker<PetriEngine::ReducingSuccessorGenerator>
                          (net, query,
                           PetriEngine::ReducingSuccessorGenerator{
                                   net, std::make_shared<LTLStubbornSet>(net, query)}) {}

        bool isSatisfied() override {
            return false;
        }
    };
}

#endif //VERIFYPN_STUBBORNTARJANMODELCHECKER_H
