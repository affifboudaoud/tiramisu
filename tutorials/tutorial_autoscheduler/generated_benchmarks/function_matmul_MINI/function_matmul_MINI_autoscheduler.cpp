#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_matmul_MINI_wrapper.h"

using namespace tiramisu;

// Path to python (please give absolute path)
const std::string py_cmd_path = "/usr/bin/python";

// Path to a script that executes the ML model (please give absolute path)
const std::string py_interface_path = "/home/afif/single/tiramisu/tutorials/tutorial_autoscheduler/model/main.py";

int main(int argc, char **argv)
{
    tiramisu::init("function_matmul_MINI");

    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 
    var l("l", 0, 16), m("m", 0, 48), n("n", 0, 32);
    //inputs
    input A("A", {l, m}, p_float64);
    input B("B", {m, n}, p_float64);
    
    //Computations
    computation comp_matmul("comp_matmul", {l, n, m}, p_float64);
    comp_matmul.set_expression(comp_matmul(l, n, m) + A(l, m)*B(m, n));

    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------
    
    
    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------
    //Input Buffers
    buffer buf_A("A", {16, 48}, p_float64, a_input);
    buffer buf_B("B", { 48, 32}, p_float64, a_input);
    buffer buf_matmul("buf_matmul", {16, 32}, p_float64, a_output);
    
    //Store inputs
    A.store_in(&buf_A);
    B.store_in(&buf_B);

    //Store computations
    comp_matmul.store_in(&buf_matmul, {l, n});
   

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

	as.sample_search_space_random_matrix("./function_matmul_MINI_explored_schedules.json", true);
	delete scheds_gen;
	delete model_eval;
	delete bs;
	return 0;
}