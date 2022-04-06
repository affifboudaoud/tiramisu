#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_heat2d_MINI_wrapper.h"


using namespace tiramisu;
// Path to python (please give absolute path)
const std::string py_cmd_path = "/usr/bin/python";

// Path to a script that executes the ML model (please give absolute path)
const std::string py_interface_path = "/home/afif/single/tiramisu/tutorials/tutorial_autoscheduler/model/main.py";

int main(int argc, char **argv)
{
    tiramisu::init("function_heat2d_MINI");
    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 
    var y_in("y_in", 0, 18), x_in("x_in", 0, 66), x("x", 1, 66 - 1), y("y", 1, 18 - 1);

    //Inputs
    input input("input", {y_in, x_in}, p_float64);

    //Computations
    computation comp_heat2d("comp_heat2d", {y, x}, p_float64);
    comp_heat2d.set_expression( input(y, x)*0.15 + (input(y, x + 1) + input(y, x -1) + input(y + 1, x) + input(y - 1, x))*0.2);

    buffer buff_input("buff_input", {18, 66}, p_float64, a_input);
    buffer buff_heat2d("buff_heat2d", {18, 66}, p_float64, a_output);

    //Store inputs
    input.store_in(&buff_input);

    //Store computations
    comp_heat2d.store_in(&buff_heat2d);

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------  
    	prepare_schedules_for_legality_checks();
	perform_full_dependency_analysis();

	const int beam_size = get_beam_size();
	const int max_depth = get_max_depth();
	declare_memory_usage();

	auto_scheduler::schedules_generator *scheds_gen = new auto_scheduler::ml_model_schedules_generator();
	auto_scheduler::evaluation_function *model_eval = new auto_scheduler::evaluate_by_learning_model(py_cmd_path, {py_interface_path});
	auto_scheduler::search_method *bs = new auto_scheduler::beam_search(beam_size, max_depth, model_eval, scheds_gen);
	auto_scheduler::auto_scheduler as(bs, model_eval);

	as.sample_search_space_random_matrix("./function_heat2d_MINI_explored_schedules.json", true);
	delete scheds_gen;
	delete model_eval;
	delete bs;
	return 0;
}