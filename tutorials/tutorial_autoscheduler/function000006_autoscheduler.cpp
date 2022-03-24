#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function000006_wrapper.h"

using namespace tiramisu;
// Path to python (please give absolute path)
const std::string py_cmd_path = "/usr/bin/python";

// Path to a script that executes the ML model (please give absolute path)
const std::string py_interface_path = "/home/afif/single/tiramisu/tutorials/tutorial_autoscheduler/model/main.py";
int main(int argc, char **argv){                
	tiramisu::init("function000006");
	var i0("i0", 0, 512), i1("i1", 0, 256), i2("i2", 0, 256);
	input icomp00("icomp00", {i0,i1,i2}, p_float64);
	input input01("input01", {i1,i2}, p_float64);
	input input02("input02", {i0,i1,i2}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(icomp00(i0, i1, i2)*input01(i1, i2)/input02(i0, i1, i2));
	buffer buf00("buf00", {512,256,256}, p_float64, a_output);
	buffer buf01("buf01", {256,256}, p_float64, a_input);
	buffer buf02("buf02", {512,256,256}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	input02.store_in(&buf02);
	comp00.store_in(&buf00);

	prepare_schedules_for_legality_checks();
	perform_full_dependency_analysis();

	const int beam_size = get_beam_size();
	const int max_depth = get_max_depth();
	declare_memory_usage();

	// An object used by search methods to generate schedules
	    auto_scheduler::schedules_generator *scheds_gen = new auto_scheduler::ml_model_schedules_generator();

	    // An evaluation function that measures execution time by compiling and executing the program
	    // auto_scheduler::evaluate_by_execution *exec_eval = new auto_scheduler::evaluate_by_execution({&buf_output, &buf_bias, &buf_src, &buf_weights},
	    //                                                                                             "function.o", "./wrapper");

	    // An evaluation function that uses an ML model to estimate speedup
	    auto_scheduler::evaluation_function *model_eval = new auto_scheduler::evaluate_by_learning_model(py_cmd_path, {py_interface_path});

	    // Two search methods : Beam Search and MCTS
	    auto_scheduler::search_method *bs = new auto_scheduler::beam_search(beam_size, max_depth, model_eval, scheds_gen);
	    //auto_scheduler::mcts *mcts = new auto_scheduler::mcts(nb_samples, topk, max_depth, model_eval, exec_eval, scheds_gen);
		
	    // Create the autoscheduler and start search
	    auto_scheduler::auto_scheduler as(bs, model_eval);
	as.sample_search_space_random_matrix("./function000006_explored_schedules.json", true);
	delete scheds_gen;
	delete model_eval;
	delete bs;
	return 0;
}
