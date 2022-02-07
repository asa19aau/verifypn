/* Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>,
 *                     Rasmus Tollund <rtollu18@student.aau.dk>
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

#include "PetriEngine/PQL/Analyze.h"

namespace PetriEngine::PQL {
    void analyze(Condition *condition, AnalysisContext& context) {
        AnalyzeVisitor visitor(context);
        condition->visit(visitor);
    }

    void analyze(Condition_ptr condition, AnalysisContext& context) {
        analyze(condition.get(), context);
    }

    void AnalyzeVisitor::_accept(NaryExpr *element) {
        for(auto& e : element->expressions())
            e->visit(*this);
    }

    void AnalyzeVisitor::_accept(MinusExpr *element) {
        (*element)[0]->visit(*this);
    }

    void AnalyzeVisitor::_accept(LiteralExpr *element) {
        // Do nothing
    }

    void AnalyzeVisitor::_accept(CommutativeExpr *element) {
        for(auto& i : element->_ids)
        {
            AnalysisContext::ResolutionResult result = _context.resolve(i.second);
            if (result.success) {
                i.first = result.offset;
            } else {
                ExprError error("Unable to resolve identifier \"" + i.second + "\"",
                                i.second.length());
                _context.reportError(error);
            }
        }
        element->NaryExpr::visit(*this);
        std::sort(element->_ids.begin(), element->_ids.end(), [](auto& a, auto& b){ return a.first < b.first; });
        std::sort(element->_exprs.begin(), element->_exprs.end(), [](auto& a, auto& b)
        {
            auto ida = std::dynamic_pointer_cast<PQL::UnfoldedIdentifierExpr>(a);
            auto idb = std::dynamic_pointer_cast<PQL::UnfoldedIdentifierExpr>(b);
            if(ida == nullptr) return false;
            if(ida && !idb) return true;
            return ida->offset() < idb->offset();
        });
    }

    uint32_t getPlace(AnalysisContext& context, const std::string& name)
    {
        AnalysisContext::ResolutionResult result = context.resolve(name);
        if (result.success) {
            return result.offset;
        } else {
            ExprError error("Unable to resolve identifier \"" + name + "\"",
                            name.length());
            context.reportError(error);
        }
        return -1;
    }

    Expr_ptr generateUnfoldedIdentifierExpr(ColoredAnalysisContext& context, std::unordered_map<uint32_t,std::string>& names, uint32_t colorIndex) {
        std::string& place = names[colorIndex];
        return std::make_shared<UnfoldedIdentifierExpr>(place, getPlace(context, place));
    }

    void AnalyzeVisitor::_accept(IdentifierExpr *element) {
        if (element->compiled()) {
            element->compiled()->visit(*this);
            return;
        }

        auto coloredContext = dynamic_cast<ColoredAnalysisContext*>(&_context);
        if(coloredContext != nullptr && coloredContext->isColored())
        {
            std::unordered_map<uint32_t,std::string> names;
            if (!coloredContext->resolvePlace(element->name(), names)) {
                ExprError error("Unable to resolve colored identifier \"" + element->name() + "\"", element->name().length());
                coloredContext->reportError(error);
            }

            if (names.size() == 1) {
                element->_compiled = generateUnfoldedIdentifierExpr(*coloredContext, names, names.begin()->first);
            } else {
                std::vector<Expr_ptr> identifiers;
                identifiers.reserve(names.size());
                for (auto& unfoldedName : names) {
                    identifiers.push_back(generateUnfoldedIdentifierExpr(*coloredContext,names,unfoldedName.first));
                }
                element->_compiled = std::make_shared<PQL::PlusExpr>(std::move(identifiers));
            }
        } else {
            element->_compiled = std::make_shared<UnfoldedIdentifierExpr>(element->name(), getPlace(_context, element->name()));
        }
        element->_compiled->visit(*this);
    }

    void AnalyzeVisitor::_accept(UnfoldedIdentifierExpr *element) {
        AnalysisContext::ResolutionResult result = _context.resolve(element->name());
        if (result.success) {
            element->_offsetInMarking = result.offset;
        } else {
            ExprError error("Unable to resolve identifier \"" + element->name() + "\"", element->name().length());
            _context.reportError(error);
        }
    }

    void AnalyzeVisitor::_accept(UnfoldedFireableCondition *element) {
        if (element->getCompiled()) element->getCompiled()->visit(*this);

        std::vector<Condition_ptr> conds;
        AnalysisContext::ResolutionResult result = _context.resolve(element->getName(), false);
        if (!result.success)
        {
            ExprError error("Unable to resolve identifier \"" + element->getName() + "\"", element->getName().length());
            _context.reportError(error);
            return;
        }

        assert(element->getName().compare(_context.net()->transitionNames()[result.offset]) == 0);
        auto preset = _context.net()->preset(result.offset);
        for(; preset.first != preset.second; ++preset.first)
        {
            assert(preset.first->place != std::numeric_limits<uint32_t>::max());
            assert(preset.first->place != -1);
            auto id = std::make_shared<UnfoldedIdentifierExpr>(_context.net()->placeNames()[preset.first->place], preset.first->place);
            auto lit = std::make_shared<LiteralExpr>(preset.first->tokens);

            if(!preset.first->inhibitor)
            {
                conds.emplace_back(std::make_shared<LessThanOrEqualCondition>(lit, id));
            }
            else if(preset.first->tokens > 0)
            {
                conds.emplace_back(std::make_shared<LessThanCondition>(id, lit));
            }
        }
        if(conds.size() == 1) element->_compiled = conds[0];
        else if (conds.empty()) {
            element->_compiled = BooleanCondition::TRUE_CONSTANT;
        }
        else element->_compiled = std::make_shared<AndCondition>(conds);
        element->_compiled->visit(*this);
    }

    void AnalyzeVisitor::_accept(FireableCondition *element) {
        if (element->getCompiled()) element->getCompiled()->visit(*this);

        auto coloredContext = dynamic_cast<ColoredAnalysisContext*>(&_context);
        if(coloredContext != nullptr && coloredContext->isColored()) {
            std::vector<std::string> names;
            if (!coloredContext->resolveTransition(element->getName(), names)) {
                ExprError error("Unable to resolve colored identifier \"" + element->getName() + "\"", element->getName().length());
                coloredContext->reportError(error);
                return;
            }
            if(names.size() < 1){
                //If the transition points to empty vector we know that it has
                //no legal bindings and can never fire
                element->_compiled = std::make_shared<BooleanCondition>(false);
                element->_compiled->visit(*this);
                return;
            }
            if (names.size() == 1) {
                element->_compiled = std::make_shared<UnfoldedFireableCondition>(names[0]);
            } else {
                std::vector<Condition_ptr> identifiers;
                for (auto& unfoldedName : names) {
                    identifiers.push_back(std::make_shared<UnfoldedFireableCondition>(unfoldedName));
                }
                element->_compiled = std::make_shared<OrCondition>(std::move(identifiers));
            }
        } else {
            element->_compiled = std::make_shared<UnfoldedFireableCondition>(element->getName());
        }
        element->_compiled->visit(*this);
    }

    void AnalyzeVisitor::_accept(CompareConjunction *element) {
        for(auto& c : element->_constraints){
            c._place = getPlace(_context, c._name);
            assert(c._place >= 0);
        }
        std::sort(std::begin(element->_constraints), std::end(element->_constraints));
    }

    void AnalyzeVisitor::_accept(CompareCondition *element) {
        (*element)[0]->visit(*this);
        (*element)[1]->visit(*this);
    }

    void AnalyzeVisitor::_accept(UntilCondition *element) {
        (*element)[0]->visit(*this);
        (*element)[1]->visit(*this);
    }

    void AnalyzeVisitor::_accept(SimpleQuantifierCondition *element) {
        (*element)[0]->visit(*this);
    }

    void AnalyzeVisitor::_accept(NotCondition *element) {
        (*element)[0]->visit(*this);
    }

    void AnalyzeVisitor::_accept(BooleanCondition *element) {
        // Do nothing
    }

    void AnalyzeVisitor::_accept(DeadlockCondition *element) {
        _context.setHasDeadlock();
    }

    void AnalyzeVisitor::_accept(KSafeCondition *element) {
        if (element->getCompiled()) element->getCompiled()->visit(*this);

        auto coloredContext = dynamic_cast<ColoredAnalysisContext*>(&_context);
        std::vector<Condition_ptr> k_safe;
        if(coloredContext != nullptr && coloredContext->isColored())
        {
            for(auto& p : coloredContext->allColoredPlaceNames())
                k_safe.emplace_back(std::make_shared<LessThanOrEqualCondition>(std::make_shared<IdentifierExpr>(p.first), element->_bound));
        }
        else
        {
            for(auto& p : _context.allPlaceNames())
                k_safe.emplace_back(std::make_shared<LessThanOrEqualCondition>(std::make_shared<UnfoldedIdentifierExpr>(p.first), element->_bound));
        }
        element->_compiled = std::make_shared<AGCondition>(std::make_shared<AndCondition>(std::move(k_safe)));
        element->_compiled->visit(*this);
    }

    void AnalyzeVisitor::_accept(LogicalCondition *element) {
        for (auto& cond : element->getOperands())
            cond->visit(*this);
    }

    void AnalyzeVisitor::_accept(QuasiLivenessCondition *element) {
        if (element->getCompiled()) element->getCompiled()->visit(*this);

        auto coloredContext = dynamic_cast<ColoredAnalysisContext*>(&_context);
        std::vector<Condition_ptr> quasi;
        if(coloredContext != nullptr && coloredContext->isColored())
        {
            for(auto& n : coloredContext->allColoredTransitionNames())
            {
                std::vector<Condition_ptr> disj;
                for(auto& tn : n.second)
                    disj.emplace_back(std::make_shared<UnfoldedFireableCondition>(tn));
                quasi.emplace_back(std::make_shared<EFCondition>(std::make_shared<OrCondition>(std::move(disj))));
            }
        }
        else
        {
            for(auto& n : _context.allTransitionNames())
            {
                quasi.emplace_back(std::make_shared<EFCondition>(std::make_shared<UnfoldedFireableCondition>(n.first)));
            }
        }
        element->_compiled = std::make_shared<AndCondition>(std::move(quasi));
        element->_compiled->visit(*this);
    }

    void AnalyzeVisitor::_accept(LivenessCondition *element) {
        if (element->getCompiled()) element->getCompiled()->visit(*this);

        auto coloredContext = dynamic_cast<ColoredAnalysisContext*>(&_context);
        std::vector<Condition_ptr> liveness;
        if(coloredContext != nullptr && coloredContext->isColored())
        {
            for(auto& n : coloredContext->allColoredTransitionNames())
            {
                std::vector<Condition_ptr> disj;
                for(auto& tn : n.second)
                    disj.emplace_back(std::make_shared<UnfoldedFireableCondition>(tn));
                liveness.emplace_back(std::make_shared<AGCondition>(std::make_shared<EFCondition>(std::make_shared<OrCondition>(std::move(disj)))));
            }
        }
        else
        {
            for(auto& n : _context.allTransitionNames())
            {
                liveness.emplace_back(std::make_shared<AGCondition>(std::make_shared<EFCondition>(std::make_shared<UnfoldedFireableCondition>(n.first))));
            }
        }
        element->_compiled = std::make_shared<AndCondition>(std::move(liveness));
        element->_compiled->visit(*this);
    }

    void AnalyzeVisitor::_accept(StableMarkingCondition *element) {
        if (element->getCompiled()) element->getCompiled()->visit(*this);

        auto coloredContext = dynamic_cast<ColoredAnalysisContext*>(&_context);
        std::vector<Condition_ptr> stable_check;
        if(coloredContext != nullptr && coloredContext->isColored())
        {
            for(auto& cpn : coloredContext->allColoredPlaceNames())
            {
                std::vector<Expr_ptr> sum;
                MarkVal init_marking = 0;
                for(auto& pn : cpn.second)
                {
                    auto id = std::make_shared<UnfoldedIdentifierExpr>(pn.second);
                    id->visit(*this);
                    init_marking += _context.net()->initial(id->offset());
                    sum.emplace_back(std::move(id));

                }
                stable_check.emplace_back(std::make_shared<AGCondition>(std::make_shared<EqualCondition>(
                        std::make_shared<PlusExpr>(std::move(sum)),
                        std::make_shared<LiteralExpr>(init_marking))));
            }
        }
        else
        {
            size_t i = 0;
            for(auto& p : _context.net()->placeNames())
            {
                stable_check.emplace_back(std::make_shared<AGCondition>(std::make_shared<EqualCondition>(
                        std::make_shared<UnfoldedIdentifierExpr>(p, i),
                        std::make_shared<LiteralExpr>(_context.net()->initial(i)))));
                ++i;
            }
        }
        element->_compiled = std::make_shared<OrCondition>(std::move(stable_check));
        element->_compiled->visit(*this);
    }

    void AnalyzeVisitor::_accept(UpperBoundsCondition *element) {
        if (element->getCompiled()) element->getCompiled()->visit(*this);

        auto coloredContext = dynamic_cast<ColoredAnalysisContext*>(&_context);
        if(coloredContext != nullptr && coloredContext->isColored())
        {
            std::vector<std::string> uplaces;
            for(auto& p : element->getPlaces())
            {
                std::unordered_map<uint32_t,std::string> names;
                if (!coloredContext->resolvePlace(p, names)) {
                    ExprError error("Unable to resolve colored identifier \"" + p + "\"", p.length());
                    coloredContext->reportError(error);
                }

                for(auto& id : names)
                {
                    uplaces.push_back(names[id.first]);
                }
            }
            element->_compiled = std::make_shared<UnfoldedUpperBoundsCondition>(uplaces);
        } else {
            element->_compiled = std::make_shared<UnfoldedUpperBoundsCondition>(element->getPlaces());
        }
        element->_compiled->visit(*this);
    }

    void AnalyzeVisitor::_accept(UnfoldedUpperBoundsCondition *element) {
        for (auto &p: element->_places) {
            AnalysisContext::ResolutionResult result = _context.resolve(p._name);
            if (result.success) {
                p._place = result.offset;
            } else {
                ExprError error("Unable to resolve identifier \"" + p._name + "\"",
                                p._name.length());
                _context.reportError(error);
            }
        }
        std::sort(element->_places.begin(), element->_places.end());
    }
}