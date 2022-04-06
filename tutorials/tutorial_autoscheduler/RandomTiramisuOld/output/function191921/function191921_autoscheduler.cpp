#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191921_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191921");
	var i0("i0", 0, 1024), i1("i1", 0, 1280), i2("i2", 0, 256);
	input icomp00("icomp00", {i1,i2}, p_float64);
	input input01("input01", {i0,i1}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(icomp00(i1, i2)*input01(i0, i1));
	buffer buf00("buf00", {1280,256}, p_float64, a_output);
	buffer buf01("buf01", {1024,1280}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	comp00.store_in(&buf00, {i1,i2});

	prepare_schedules_for_legality_checks();
	perform_full_dependency_analysis();

	const int beam_size = get_beam_size();
	const int max_depth = get_max_depth();
	declare_memory_usage();

	auto_scheduler::schedules_generator *scheds_gen = new auto_scheduler::ml_model_schedules_generator();
	auto_scheduler::evaluate_by_execution *exec_eval = new auto_scheduler::evaluate_by_execution({&buf00,&buf01}, "function191921.o", "./function191921_wrapper");
	auto_scheduler::search_method *bs = new auto_scheduler::beam_search(beam_size, max_depth, exec_eval, scheds_gen);
	auto_scheduler::auto_scheduler as(bs, exec_eval);
	as.set_exec_evaluator(exec_eval);
	as.sample_search_space("./function191921_explored_schedules.json", true);
	delete scheds_gen;
	delete exec_eval;
	delete bs;
	return 0;
}