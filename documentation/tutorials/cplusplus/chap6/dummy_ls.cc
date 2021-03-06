// Copyright 2011-2014 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// Dummy Local Search to understand the behavior of LSN Operators.

#include "constraint_solver/constraint_solveri.h"
#include "constraint_solver/constraint_solver.h"

DEFINE_int64(n, 4, "Size of the problem");
DEFINE_bool(initial_phase, true, "Do we use an initial phase to produce"
                                              "the initial solution?");
DEFINE_int64(ls_time_limit, 10000, "LS time limit (in ms)");
DEFINE_int64(ls_branches_limit, 10000, "LS branches limit");
DEFINE_int64(ls_failures_limit, 10000, "LS failures limit");
DEFINE_int64(ls_solutions_limit, 1, "LS solutions limit");
DEFINE_bool(print_intermediate_solutions, true,
            "Add a search log for the objective?");

namespace operations_research {

class DecreaseOneVar: public IntVarLocalSearchOperator {
  public:
    explicit DecreaseOneVar(const std::vector<IntVar*>& variables)
            : IntVarLocalSearchOperator(variables),
            variable_index_(0) {
              VLOG(2) << "Creation of DecreaseOneVar Local Search Operator";
            }
    virtual ~DecreaseOneVar() {
      VLOG(2) << "Destruction of DecreaseOneVar Local Search Operator";
    }

  protected:
    // Make a neighbor assigning one variable to its target value.
    virtual bool MakeOneNeighbor() {
      if (variable_index_ ==  Size()) {
        VLOG(1) << "End of neighborhood search";
        return false;
      }

      const int64 current_value = Value(variable_index_);
        SetValue(variable_index_, current_value  - 1);
        VLOG(1) << "Current value of variable index = "
                << variable_index_
                << " set to "
                << current_value  - 1;
        variable_index_ = variable_index_ + 1;
        return true;
    }

  private:
    virtual void OnStart() {
      VLOG(1) << "Start new neighborhood search";
        variable_index_ = 0;
    }
    int64 variable_index_;
};

void DummyLS(const int64 n, const bool init_phase) {
    CHECK_GE(n, 2) << "size of problem (n) must be greater or equal than 2";
    LOG(INFO) << "Dummy Local Search "
              << (init_phase ? "with initial phase" : "with initial solution");

    Solver s("Dummy LS");

    vector<IntVar*> vars;
    s.MakeIntVarArray(n, 0, n-1, &vars);
    IntVar* const sum_var = s.MakeSum(vars)->Var();
    OptimizeVar* const obj = s.MakeMinimize(sum_var, 1);

    // unique constraint x_0 >= 1
    s.AddConstraint(s.MakeGreaterOrEqual(vars[0], 1));

    // initial phase builder
    DecisionBuilder * db = NULL;
    // initial solution
    Assignment * const initial_solution = s.MakeAssignment();

    if (init_phase) {
        db =
            s.MakePhase(vars,
                        Solver::CHOOSE_FIRST_UNBOUND,
                        Solver::ASSIGN_MAX_VALUE);

    } else {
        initial_solution->Add(vars);
        for (int i = 0; i < n; ++i) {
            if (i % 2 == 0) {
                initial_solution->SetValue(vars[i], n - 1);
            } else {
                initial_solution->SetValue(vars[i], n - 2);
            }
        }
    }

    //  IntVarLocalSearchOperator
    DecreaseOneVar one_var_ls(vars);
    LocalSearchPhaseParameters* ls_params = NULL;

    SearchLimit * const limit = s.MakeLimit(FLAGS_ls_time_limit,
                                            FLAGS_ls_branches_limit,
                                            FLAGS_ls_failures_limit,
                                            FLAGS_ls_solutions_limit);


    DecisionBuilder* ls = NULL;



    if (init_phase) {
      ls_params = s.MakeLocalSearchPhaseParameters(&one_var_ls, db, limit);
        ls = s.MakeLocalSearchPhase(vars, db, ls_params);
    } else {
      ls_params = s.MakeLocalSearchPhaseParameters(&one_var_ls, NULL, limit);
        ls = s.MakeLocalSearchPhase(initial_solution, ls_params);
    }

    SolutionCollector* const collector = s.MakeLastSolutionCollector();
    collector->Add(vars);
    collector->AddObjective(sum_var);

    SearchMonitor* log = NULL;
    if (FLAGS_print_intermediate_solutions) {
      log = s.MakeSearchLog(1000, obj);
    }

    if (s.Solve(ls,  collector, obj, log)) {
    LOG(INFO) << "Objective value = " << collector->objective_value(0);
    } else {
      LG << "No solution...";
    }
}

}  //  namespace operations_research

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    operations_research::DummyLS(FLAGS_n, FLAGS_initial_phase);
    return 0;
}

