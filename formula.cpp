#include <iostream>
#include "formula.hpp"

Formula::Formula(const Cudd &mgr) : mgr(mgr) {}

Formula::Formula(const Cudd &mgr, QuantifiedVariablesManager &qvmgr) : QuantifiedVariablesManipulator(qvmgr), mgr(mgr) {}

BDD Formula::getMatrix() const {
    return matrix;
}

void Formula::setMatrix(const BDD &matrix) {
    if (mgr.getManager() != matrix.manager()) {
        throw "Managers are fucking different mate";
    }
    this->matrix = matrix;
}

VariableSet Formula::getSupportSet() {
    VariableSet supportSet;
    for (unsigned int index : matrix.SupportIndices()) {
        supportSet.insert(Variable(index, mgr));
    }
    return supportSet;
}

void Formula::removeUnusedVars() {
    VariableSet usedVars = getSupportSet();

    VariableSet varsToRemove;
    for (Variable uVar : getUnivVars()) {
        if (!usedVars.contains(uVar)) { // !usedVars.contains(uVar) in c++20
            varsToRemove.insert(uVar);
        }
    }

    for (Variable eVar : getExistVars()) {
        if (usedVars.count(eVar) == 0) { // !usedVars.contains(eVar) in c++20
            varsToRemove.insert(eVar);
        }
    }

    for (Variable varToRemove : varsToRemove) {
        removeVar(varToRemove);
    }
}

void Formula::eliminateUnivVar(Variable uVarToEliminate) {
    // TODO duplicate only those that are in the bdd????? -> pozri ako som zrobil pushUnivVar ci to fakt treba
    VariableSet eVarsToDuplicate;
    VariableSet supportSet = getSupportSet();
    VariableSet dependentVars = getUnivVarDependencies(uVarToEliminate);
    for (Variable dependentVar : dependentVars) {
        if (supportSet.contains(dependentVar)) {
            eVarsToDuplicate.insert(dependentVar);
            removeDependency(dependentVar, uVarToEliminate);
        }
    }

    removeUnivVar(uVarToEliminate);
    
    // pair used for replacing existential variables that depend on uVarToEliminate with new ones
    std::vector<BDD> varsToBeReplaced;
    std::vector<BDD> varsToReplaceWith;

    std::cout << "Duplicating vars ";
    for (Variable eVarToDuplicate : eVarsToDuplicate) {
        std::cout << eVarToDuplicate.getId() << " ";
        Variable newExistVar = eVarToDuplicate.newVarAtSameLevel();
        addExistVar(newExistVar, getExistVarDependencies(eVarToDuplicate));
        varsToBeReplaced.push_back(eVarToDuplicate);
        varsToReplaceWith.push_back(newExistVar);
    }
    std::cout << std::endl;


    // TODO what is the FUCKING difference between constrain and restrict
    std::cout << "Creating BDDs" << std::endl;
    // uVarToEliminate=false where we have old existential variables
    BDD f1 = matrix.Restrict(!uVarToEliminate.getBDD());
    std::cout << "Restriction 1 finished" << std::endl;
    // uVarToEliminate=true where we have new existential variables
    BDD f2 = matrix.Restrict(uVarToEliminate);
    std::cout << "Restriction 2 finished" << std::endl;
    f2 = f2.SwapVariables(varsToBeReplaced, varsToReplaceWith);
    std::cout << "Replacing finished" << std::endl;
    // get their conjuction and thus remove univ_id from the formula
    setMatrix(f1 & f2);
    std::cout << "BDD created" << std::endl;
}

/*
bool Formula::isMatrixOne() {
    return matrix == bddtrue;
}

bool Formula::isMatrixZero() {
    return matrix == bddfalse;
}
*/