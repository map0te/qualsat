#include <stdio.h>
#include <iostream>
#include <ctime>
#include <vector>

enum { UNSAT = 0, SAT = 1 };
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

typedef int Lit;

class Clause {
    private:
        std::vector<Lit> lit;
    public:
        void push_back(Lit l) {lit.push_back(l);};
        int size() {return lit.size();};
        int operator[] (int idx) {return lit[idx];};
};

class Solver {
    private:
        int nVars, nClauses;
        int processed;
        std::vector<Clause> formula;
        std::vector<int> assignment;
        std::vector<bool> forced;
        std::vector<Lit> trail;
        std::vector<Clause> reason;
        std::vector<std::vector<int>> watched;
        int assign(Lit lit, Clause reason, bool forced);
        int add_clause(Clause clause);
        int add_watch(Lit lit, int clauseIdx);
    public:
        Solver();
        int parse_cnf(const char* filename);
        int print_cnf();
};

Solver::Solver() {
    nVars = nClauses = processed = 0;
}

int Solver::assign(Lit lit, Clause reason, bool forced) {
    this->assignment[abs(lit)] = sgn(lit);
    this->forced[abs(lit)] = forced;
    this->reason.push_back(reason);
    this->trail.push_back(lit);
    return 0;
}

int Solver::add_clause(Clause clause) {
    if (clause.size() > 1) {
        add_watch(clause[0], formula.size());
        add_watch(clause[1], formula.size());
    }
    formula.push_back(clause);
    return 0;
}

int Solver::add_watch(Lit lit, int clauseIdx) {
    watched[abs(lit)].push_back(clauseIdx);
    return 0;
}

int Solver::parse_cnf(const char* filename) {
    FILE* F;
    F = fopen(filename, "r");
    if (F == 0) {
        printf("error opening file %s\n", filename);
        printf("exiting...\n");
        exit(1);
    }

    printf("parsing \"%s\"...\n", filename);
    char c = getc(F);
    while (c == 'c') {
        do { c = getc(F); } while (c != '\n');
        c = getc(F);
    }
    ungetc(c, F);
    if (fscanf(F, "p cnf %d %d\n", &nVars, &nClauses) != 2) {
        printf("failed to find p cnf header\n");
        printf("exiting...\n");
        exit(1);
    }
    assignment.resize(nVars);
    watched.resize(nVars);
    for (int i = 0; i < nClauses; i++) {
        Lit lit;
        Clause clause;
        fscanf(F, "%d", &lit);
        do {
            clause.push_back(lit);
            fscanf(F, "%d", &lit);
        } while (lit);
        add_clause(clause);
    }
    return 0;
}

int Solver::print_cnf() {
    for (int i = 0; i < nClauses; i++) {
        for (int j = 0; j < formula[i].size(); j++) {
            std::cout << formula[i][j] << " ";
        }
        std::cout << std::endl;
    }
    return 0;
};

int main(int argc, char** argv) {
    Solver solver;
    solver.parse_cnf(argv[1]);
    solver.print_cnf();
}