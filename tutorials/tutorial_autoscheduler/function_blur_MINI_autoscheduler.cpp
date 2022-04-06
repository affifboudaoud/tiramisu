#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_blur_MINI_wrapper.h"


using namespace tiramisu;
// Path to python (please give absolute path)
const std::string py_cmd_path = "/usr/bin/python";

// Path to a script that executes the ML model (please give absolute path)
const std::string py_interface_path = "/home/afif/single/tiramisu/tutorials/tutorial_autoscheduler/model/main.py";

int main(int argc, char **argv)
{
    tiramisu::init("function_blur_MINI");

    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 
    var xi("xi", 0, 34), yi("yi", 0, 18), ci("ci", 0, 5);
    var x("x", 1, 34-1), y("y", 1, 18-1), c("c", 1, 5-1);

    //inputs
    input input_img("input_img", {ci, yi, xi}, p_float64);

    //Computations
    computation comp_blur("comp_blur", {c, y, x}, (input_img(c, y + 1, x - 1) + input_img(c, y + 1, x) + input_img(c, y + 1, x + 1) + 
                                         input_img(c, y, x - 1)     + input_img(c, y, x)     + input_img(c, y, x + 1) + 
                                         input_img(c, y - 1, x - 1) + input_img(c, y - 1, x) + input_img(c, y - 1, x + 1))*0.111111);
    
    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------


    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------
    //Buffers
    buffer input_buf("input_buf", {5, 18, 34}, p_float64, a_input);
    buffer output_buf("output_buf", {5,18, 34}, p_float64, a_output);

    //Store inputs
    input_img.store_in(&input_buf);

    //Store computations
    comp_blur.store_in(&output_buf);
 
    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------
    	prepare_schedules_for_legality_checks();
	perform_full_dependency_analysis();

	const int beam_size = get_beam_size();
	const int max_depth = get_max_depth();
	declare_memory_usage();

	auto_scheduler::schedules_generator *scheds_gen = new auto_scheduler::ml_model_schedules_generator();
	//auto_scheduler::evaluate_by_execution *exec_eval = new auto_scheduler::evaluate_by_execution({&input_buf, &output_buf}, "function_blur_MINI.o", "./function_blur_MINI_wrapper");
	auto_scheduler::evaluation_function *model_eval = new auto_scheduler::evaluate_by_learning_model(py_cmd_path, {py_interface_path});

	auto_scheduler::search_method *bs = new auto_scheduler::beam_search(beam_size, max_depth, model_eval, scheds_gen);
	
	auto_scheduler::auto_scheduler as(bs, model_eval);
	as.sample_search_space_random_matrix("./function_blur_MINI_explored_schedules.json", true);
	delete scheds_gen;
	delete model_eval;
	delete bs;
	return 0;
}