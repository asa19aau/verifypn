/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
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

#include <vector>
#include <unordered_map>

#include "ColoredNetStructures.h"
#include "EquivalenceClass.h"

namespace PetriEngine {

    typedef std::unordered_map<std::string, Colored::ColorType*> ColorTypeMap;

    class NaiveBindingGenerator {
    public:
        class Iterator {
        private:
            NaiveBindingGenerator* _generator;
            
        public:
            Iterator(NaiveBindingGenerator* generator);
            
            bool operator==(Iterator& other);
            bool operator!=(Iterator& other);
            Iterator& operator++();
            const Colored::ExpressionContext::BindingMap operator++(int);
            Colored::ExpressionContext::BindingMap& operator*();
        };
    private:
        Colored::GuardExpression_ptr _expr;
        Colored::ExpressionContext::BindingMap _bindings;
        ColorTypeMap& _colorTypes;
        
        bool eval();
        
    public:
        NaiveBindingGenerator(const Colored::Transition& transition,
                ColorTypeMap& colorTypes);

        Colored::ExpressionContext::BindingMap& nextBinding();
        Colored::ExpressionContext::BindingMap& currentBinding();
        bool isInitial() const;
        Iterator begin();
        Iterator end();
    };


    class FixpointBindingGenerator {
    public:
        class Iterator {
        private:
            FixpointBindingGenerator* _generator;
                        
        public:
            Iterator(FixpointBindingGenerator* generator);
            
            bool operator==(Iterator& other);
            bool operator!=(Iterator& other);
            Iterator& operator++();
            const Colored::ExpressionContext::BindingMap operator++(int);
            Colored::ExpressionContext::BindingMap& operator*();
        };
    private:
        const Colored::GuardExpression_ptr &_expr;
        Colored::ExpressionContext::BindingMap _bindings;
        std::vector<std::vector<std::vector<uint32_t>>> _symmetric_var_combinations;
        const ColorTypeMap& _colorTypes;
        const Colored::Transition &_transition;
        const std::vector<std::set<const Colored::Variable *>>& _symmetric_vars;
        PetriEngine::Colored::ExpressionContext::BindingMap::iterator _bindingIterator;
        bool _isDone;
        bool _noValidBindings;
        uint32_t _nextIndex = 0;
        uint32_t _currentOuterId = 0;
        uint32_t _currentInnerId = 0;
        uint32_t _symmetric_vars_set = 0;
        
        bool eval() const;
        bool assignSymmetricVars();
        void generateCombinations(
            uint32_t options,
            uint32_t samples,
            std::vector<std::vector<uint32_t>> &result,
            std::vector<uint32_t> &current) const;
        
    public:
        FixpointBindingGenerator(const Colored::Transition &transition,
                const ColorTypeMap& colorTypes,  const std::vector<std::set<const Colored::Variable *>>& symmetric_vars);

        FixpointBindingGenerator(const FixpointBindingGenerator& ) = default;
        
        FixpointBindingGenerator operator= (const FixpointBindingGenerator& b) const{
            return FixpointBindingGenerator(b);
        }

        Colored::ExpressionContext::BindingMap& nextBinding();
        Colored::ExpressionContext::BindingMap& currentBinding();
        bool isInitial() const;
        Iterator begin();
        Iterator end();
    };    
}