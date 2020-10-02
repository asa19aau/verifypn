//
// Created by Simon Mejlby Virenfeldt on 25/09/2020.
//

#include "LTL/ProductSuccessorGenerator.h"

namespace LTL {

    void ProductSuccessorGenerator::prepare(const LTL::Structures::ProductState *state) {
        SuccessorGenerator::prepare(state);
        buchi.prepare(state->getBuchiState());
        fresh_marking = true;
    }

    bool ProductSuccessorGenerator::next(LTL::Structures::ProductState &state) {
        if (fresh_marking) {
            fresh_marking = false;
            if (!SuccessorGenerator::next(state)) {
                return false;
            }
        }
        if (next_buchi_succ(state)) {
            return true;
        }
        // No valid transition in Büchi automaton for current marking;
        // Try next marking(s) and see if we find a successor.
        else {
            while (SuccessorGenerator::next(state)) {
                if (next_buchi_succ(state)) {
                    return true;
                }
            }
            return false;
        }
    }

    bool ProductSuccessorGenerator::isAccepting(const LTL::Structures::ProductState &state) {
        return buchi.is_accepting(state.getBuchiState());
    }

    bool ProductSuccessorGenerator::next_buchi_succ(LTL::Structures::ProductState &state) {
        size_t tmp;
        while (buchi.next(tmp, cond)) {
            if (guard_valid(state, cond)) {
                state.setBuchiState(tmp);
                return true;
            }
        }
        return false;
    }

    bool ProductSuccessorGenerator::guard_valid(const PetriEngine::Structures::State &state, bdd bdd) {
        EvaluationContext ctx{state.marking(), &_net};
        while (!(bdd == bddtrue || bdd == bddfalse)) {
            size_t var = bdd_var(bdd);
            Condition::Result res = buchi.getExpression(var)->evaluate(ctx);
            switch (res) {
                case Condition::RUNKNOWN:
                    std::cerr << "Unexpected unknown answer from evaluating query!\n";
                    assert(false);
                    exit(0);
                    break;
                case Condition::RFALSE:
                    bdd = bdd_low(bdd);
                    break;
                case Condition::RTRUE:
                    bdd = bdd_high(bdd);
                    break;
            }
        }
        return bdd == bddtrue;
    }

}
