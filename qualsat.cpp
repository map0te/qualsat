#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <ctime>
#include <vector>
#include <list>

enum { UNKNOWN = 0, SAT = 10, UNSAT = 20 };
enum { NOTFALSE = 0, FALSE = 1, IMPLIED = 2, MARKED = 2};

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

class Clause {
    private:
        std::vector<int> lit;
    public:
        void push_back(int l) {lit.push_back(l);};
        int size() {return lit.size();};
        int& operator[] (int idx) {return lit[idx];};
};

class Watch {
    private:
        int nVars;
        std::vector<std::list<int>> watching;
    public:
        Watch() {;};
        Watch(int nVars) { 
            this->nVars = nVars;
            watching.resize(2*nVars+1); 
        };
        std::list<int>& operator[] (int lit) {
            return watching[nVars+lit];
        };
};

class Trail {
    public:
        Trail() {;};
        Trail(int nVars) {
            nProcessed = 0;
            nImplied = 0;
        };
        int nProcessed;
        int nImplied;
        std::vector<int> literal;
        std::vector<int> reason;
        int size() { return literal.size(); };
        static bool cmp(const int lhs, const int rhs) {
            return abs(lhs) < abs(rhs);
        };
        void print_result() {
            std::sort(literal.begin(), literal.end(), cmp);
            for (auto it : literal) {
                printf("%d ", it);
            }
            printf("0\n");
        };
};

class Model {
    private:
        int nVars;
        std::vector<int> assignment;
    public:
        Model() {;};
        Model(int nVars) {
            this->nVars = nVars;
            assignment.resize(2*nVars+1);
        }
        int& operator[] (int lit) {
            return assignment[nVars+lit];
        };
};

class Solver {
    private:
        // formula data structures
        int nVars, nClauses;
        Trail trail;
        Watch watched;
        Model model;
        std::vector<Clause> formula;

        // conflict analysis

        // statistics
        int nConflicts, nRestarts;

        // functions
        int add_clause(Clause clause);
        int add_watch(int lit, int clauseIdx);
        int analyze(int clauseIdx);
        int assign(int lit, int reasonIdx, bool forced);
        int backtrack();
        int propagate();
        int unassign();

    public:
        Solver();
        int parse_cnf(const char* filename);
        int print_cnf();
        void print_result() {
            trail.print_result();
        }
        int solve();
};

Solver::Solver() {
    nVars = nClauses = 0;
    nConflicts, nRestarts = 0;
}

int Solver::add_watch(int lit, int clauseIdx) {
    watched[lit].push_back(clauseIdx);
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

int Solver::assign(int lit, int reasonIdx, bool forced) {
    model[-lit] = forced ? IMPLIED : FALSE;
    trail.literal.push_back(-lit);
    trail.reason.push_back(reasonIdx);
    return 0;
}

int Solver::unassign() {
    int lit = trail.literal.back();
    model[lit] = NOTFALSE;
    trail.literal.pop_back();
    trail.reason.pop_back();
    trail.nProcessed--;
    return 0;
}

int Solver::propagate() {
    int forced;
    if (trail.size()) {
        forced = (formula[trail.reason[trail.nProcessed]].size() != 1);
    }
    while (trail.nProcessed < trail.size()) {
        int lit = trail.literal[trail.nProcessed];
        trail.nProcessed++;
        for (auto it = watched[lit].begin(); it != watched[lit].end();) {
            int clauseIdx = *it;
            Clause& clause = formula[clauseIdx];
            bool unit = true;
            if (lit == clause[0]) {
                std::swap(clause[0], clause[1]);
            }
            for (int i = 2; i < clause.size(); i++) {
                if (!model[clause[i]]) {
                    std::swap(clause[1], clause[i]);
                    add_watch(clause[1], clauseIdx);
                    it = watched[lit].erase(it);
                    unit = false;
                    break;
                }
            }
            if (unit) {
                if (model[-clause[0]]) {it++; continue;}
                else if (!model[clause[0]]) {
                    assign(clause[0], clauseIdx, forced);
                    it++;
                }
                else {
                    //if (forced) return UNSAT;
                    //int learnedIdx = analyze(clauseIdx);
                    //if (formula[learnedIdx].size() == 1) {
                    return UNKNOWN;
                }
            }
        }
    }
    if (forced) { trail.nImplied = trail.nProcessed; }
    return SAT;
}

/*int Solver::analyze(int clauseIdx) {
    nConflicts++;
    for (auto it : formula[clauseIdx]) {
        if (model[*it] != IMPLIED) {
            model[*it] = MARKED;
        }
    }
}*/

int Solver::backtrack() {
    while (trail.size() && formula[trail.reason.back()].size() != 0) {
        unassign();
    }
    if (trail.size()) {
        int lit = trail.literal.back();
        unassign();
        return lit;
    } else {
        return nClauses;
    }
}

int Solver::solve() {
    if (propagate() == UNSAT) return UNSAT;
    for (;;) {
        int decision = 1;
        while (decision <= nVars && (model[decision] || model[-decision])) {
            decision++;
        }
        if (decision > nVars) {
            return SAT;
        }
        assign(decision, nClauses, false);
        if (propagate() == SAT) {
            continue;
        } else {
            B:;
            int lit = backtrack();
            if (lit == nClauses) {
                return UNSAT;
            }
            if (lit > 0) {
                goto B;
            }
            assign(lit, nClauses, false);
            int res = propagate();
            if (res == UNKNOWN) { goto B; }
        }
    }
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
    printf("found \'p cnf %d %d\' header\n", nVars, nClauses);
    trail = Trail(nVars);
    watched = Watch(nVars);
    model = Model(nVars);
    for (int i = 0; i < nClauses; i++) {
        int lit;
        Clause clause;
        // construct clause
        fscanf(F, "%d", &lit);
        while (lit) {
            clause.push_back(lit);
            fscanf(F, "%d", &lit);
        }
        // push to formula
        add_clause(clause);
        // check for unit 
        if (clause.size() == 0) {
            return UNSAT;
        } else if (clause.size() == 1) {
            if (model[clause[0]]) {
                return UNSAT;
            } else {
                assign(clause[0], formula.size()-1, true);
            }
        }   
    }
    Clause empty;
    add_clause(empty);
    return UNKNOWN;
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
    if (solver.parse_cnf(argv[1]) == UNSAT) {printf("\nUNSATISFIABLE\n"); return 0;};
    if (solver.solve() == UNSAT) {printf("\nUNSATISFIABLE\n"); return 0;};
    printf("\n");
    solver.print_result();
    printf("\nSATISFIABLE\n");
}