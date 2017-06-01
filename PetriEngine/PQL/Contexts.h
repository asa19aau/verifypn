/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef CONTEXTS_H
#define CONTEXTS_H


#include <string>
#include <vector>
#include <list>
#include <map>
#include <chrono>

#include "../PetriNet.h"
#include "PetriEngine/Simplification/LPCache.h"
#include "PQL.h"

namespace PetriEngine {
    namespace PQL {

        /** Context provided for context analysis */
        class AnalysisContext {
        protected:
            std::map<std::string, uint32_t> _places;
            std::vector<ExprError> _errors;
        public:

            /** A resolution result */
            struct ResolutionResult {
                /** Offset in relevant vector */
                int offset;
                /** True, if the resolution was successful */
                bool success;
            };

            AnalysisContext(const std::map<std::string, uint32_t>& places)
            : _places(places) {
            }
            
            virtual void setHasDeadlock(){};

            /** Resolve an identifier */
            virtual ResolutionResult resolve(std::string identifier) {
                ResolutionResult result;
                result.offset = -1;
                result.success = false;
                auto it = _places.find(identifier);
                if(it != _places.end())
                {
                    result.offset = (int)it->second;
                    result.success = true;
                    return result;
                }                
                return result;
            }

            /** Report error */
            void reportError(const ExprError& error) {
                _errors.push_back(error);
            }

            /** Get list of errors */
            const std::vector<ExprError>& errors() const {
                return _errors;
            }
        };

        /** Context provided for evalation */
        class EvaluationContext {
        public:

            /** Create evaluation context, this doesn't take ownership */
            EvaluationContext(const MarkVal* marking,
                    const PetriNet* net) {
                _marking = marking;
                _net = net;
            }

            const MarkVal* marking() const {
                return _marking;
            }

            const PetriNet* net() const {
                return _net;
            }
        private:
            const MarkVal* _marking;
            const PetriNet* _net;
        };

        /** Context for distance computation */
        class DistanceContext : public EvaluationContext {
        public:

            DistanceContext(const PetriNet* net,
                    const MarkVal* marking)
            : EvaluationContext(marking, net) {
                _negated = false;
            }


            void negate() {
                _negated = !_negated;
            }

            bool negated() const {
                return _negated;
            }

        private:
            bool _negated;
        };

        /** Context for condition to TAPAAL export */
        class TAPAALConditionExportContext {
        public:
            bool failed;
            std::string netName;
        };

        class SimplificationContext {
        public:

            SimplificationContext(const MarkVal* marking,
                    const PetriNet* net, uint32_t queryTimeout, uint32_t lpTimeout,
                    LPCache* cache)
                    : _queryTimeout(queryTimeout), _lpTimeout(lpTimeout) {
                _negated = false;
                _marking = marking;
                _net = net;
                _start = std::chrono::high_resolution_clock::now();
                _cache = cache;
            }

            const MarkVal* marking() const {
                return _marking;
            }

            const PetriNet* net() const {
                return _net;
            }

            void negate() {
                _negated = !_negated;
            }

            bool negated() const {
                return _negated;
            }
            
            void setNegate(bool b){
                _negated = b;
            }
            
            double getReductionTime(){
                // duration in seconds
                auto end = std::chrono::high_resolution_clock::now();
                return (std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count())*0.000001;
            }
            
            bool timeout() const {
                auto end = std::chrono::high_resolution_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _start);
                return (diff.count() >= _queryTimeout);
            }
            
            uint32_t getLpTimeout() const {
                return _lpTimeout;
            }
            
            LPCache* cache() const
            {
                return _cache;
            }

        private:
            bool _negated;
            const MarkVal* _marking;
            const PetriNet* _net;
            uint32_t _queryTimeout, _lpTimeout;
            std::chrono::high_resolution_clock::time_point _start;
            LPCache* _cache;
        };

    } // PQL
} // PetriEngine

#endif // CONTEXTS_H
