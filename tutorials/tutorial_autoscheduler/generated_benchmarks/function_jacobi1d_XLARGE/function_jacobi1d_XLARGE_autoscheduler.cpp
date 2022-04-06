#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_jacobi1d_XLARGE_wrapper.h"

using namespace tiramisu;
// Path to python (please give absolute path)
const std::string py_cmd_path = "/usr/bin/python";

// Path to a script that executes the ML model (please give absolute path)
const std::string py_interface_path = "/home/afif/single/tiramisu/tutorials/tutorial_autoscheduler/model/main.py";


int main(int argc, char **argv)
{
    tiramisu::init("function_jacobi1d_XLARGE");

    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 

    //Iteration variables    
    var i_f("i_f", 0, 4098);
    var t("t", 0, 2048), i("i", 1, 4098-1);
    
    //inputs
    input A("A", {i_f}, p_float64);


    //Computations
    computation A_out("A_out", {t,i}, (A(i-1) + A(i) + A(i + 1))*0.33333);

    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------
    
    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------
    //Input Buffers
    buffer b_A("b_A", {4098}, p_float64, a_output);    

    //Store inputs
    A.store_in(&b_A);

    //Store computations
    A_out.store_in(&b_A, {i});

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

	as.sample_search_space_random_matrix("./function_jacobi1d_XLARGE_explored_schedules.json", true);
	delete scheds_gen;
	delete model_eval;
	delete bs;
	return 0;
}