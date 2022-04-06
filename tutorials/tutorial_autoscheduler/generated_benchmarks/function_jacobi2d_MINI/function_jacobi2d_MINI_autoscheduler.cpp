#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_jacobi2d_MINI_wrapper.h"

using namespace tiramisu;

// Path to python (please give absolute path)
const std::string py_cmd_path = "/usr/bin/python";

// Path to a script that executes the ML model (please give absolute path)
const std::string py_interface_path = "/home/afif/single/tiramisu/tutorials/tutorial_autoscheduler/model/main.py";

int main(int argc, char **argv)
{
    tiramisu::init("function_jacobi2d_MINI");

    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 

    //Iteration variables    
    var i_f("i_f", 0, 50), j_f("j_f", 0, 50);
    var t("t", 0, 64), i("i", 1, 50-1), j("j", 1, 50-1);
    
    //inputs
    input A("A", {i_f, j_f}, p_float64);

    //Computations
    computation comp_A_out("comp_A_out", {t,i,j}, (A(i, j) + A(i, j-1) + A(i, 1+j) + A(1+i, j) + A(i-1, j))*0.2);

    //Input Buffers
    buffer b_A("b_A", {50,50}, p_float64, a_output);    

    //Store inputs
    A.store_in(&b_A);


    //Store computations
    comp_A_out.store_in(&b_A, {i,j});

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

	as.sample_search_space_random_matrix("./function_jacobi2d_MINI_explored_schedules.json", true);
	delete scheds_gen;
	delete model_eval;
	delete bs;
	return 0;
}