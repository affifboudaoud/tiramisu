#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191923_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191923");
	var i0("i0", 0, 64), i1("i1", 0, 64), i2("i2", 0, 64), i3("i3", 1, 129), i4("i4", 1, 385), i5("i5", 1, 385), i3_p1("i3_p1", 0, 130), i4_p1("i4_p1", 0, 386), i5_p1("i5_p1", 0, 386);
	input icomp00("icomp00", {i3_p1,i4_p1,i5_p1}, p_float64);
	computation comp00("comp00", {i0,i1,i2,i3,i4,i5},  p_float64);
	comp00.set_expression(icomp00(i3, i4, i5) + expr(0.900)*icomp00(i3, i4, i5 - 1) + icomp00(i3, i4, i5 + 1)*icomp00(i3, i4 - 1, i5)*icomp00(i3, i4 + 1, i5)*icomp00(i3 + 1, i4, i5) + icomp00(i3 - 1, i4, i5));
	buffer buf00("buf00", {130,386,386}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00, {i3,i4,i5});

	prepare_schedules_for_legality_checks();
	perform_full_dependency_analysis();

	const int beam_size = get_beam_size();
	const int max_depth = get_max_depth();
	declare_memory_usage();

	auto_scheduler::schedules_generator *scheds_gen = new auto_scheduler::ml_model_schedules_generator();
	auto_scheduler::evaluate_by_execution *exec_eval = new auto_scheduler::evaluate_by_execution({&buf00}, "function191923.o", "./function191923_wrapper");
	auto_scheduler::search_method *bs = new auto_scheduler::beam_search(beam_size, max_depth, exec_eval, scheds_gen);
	auto_scheduler::auto_scheduler as(bs, exec_eval);
	as.set_exec_evaluator(exec_eval);
	as.sample_search_space("./function191923_explored_schedules.json", true);
	delete scheds_gen;
	delete exec_eval;
	delete bs;
	return 0;
}