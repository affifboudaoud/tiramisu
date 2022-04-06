#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_heat3d_MEDIUM_wrapper.h"

using namespace tiramisu;
// Path to python (please give absolute path)
const std::string py_cmd_path = "/usr/bin/python";

// Path to a script that executes the ML model (please give absolute path)
const std::string py_interface_path = "/home/afif/single/tiramisu/tutorials/tutorial_autoscheduler/model/main.py";

int main(int argc, char **argv)
{
    tiramisu::init("function_heat3d_MEDIUM");

    // -------------------------------------------------------
    // Layer I
    // -------------------------------------------------------     
    //for heat3d_init
    var x_in("x_in", 0, 50), y_in("y_in", 0, 50), z_in("z_in", 0, 50), t_in("t_in", 0, 256);
    //for heat3d_c
    var x("x",1,50-1), y("y",1,50-1), z("z",1,50-1), t("t",0,256);

    computation comp_heat3d("heat3d",{t,z,y,x},p_float64);
    comp_heat3d.set_expression(
		comp_heat3d(t,z,y,x) +
		    (comp_heat3d(t,z-1,y,x) - comp_heat3d(t,z,y,x)*2.0 + comp_heat3d(t,z+1,y,x)
			 + comp_heat3d(t,z,y-1,x) - comp_heat3d(t,z,y,x)*2.0 + comp_heat3d(t,z,y+1,x)
			 + comp_heat3d(t,z,y,x-1) - comp_heat3d(t,z,y,x)*2.0 + comp_heat3d(t,z,y,x+1))*0.125);
  
    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------    

    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------    
    //buffers
    buffer b_out("b_out",{50,50,50},p_float64,a_output);
    
    //Store inputs

    //Store computations  
    comp_heat3d.store_in(&b_out,{z,y,x});

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

	as.sample_search_space_random_matrix("./function_heat3d_MEDIUM_explored_schedules.json", true);
	delete scheds_gen;
	delete model_eval;
	delete bs;
	return 0;
}