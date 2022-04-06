#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function7522636_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function7522636");
	var i0("i0", 0, 128), i1("i1", 1, 65), i2("i2", 1, 449), i3("i3", 1, 449), i1_p1("i1_p1", 0, 66), i2_p1("i2_p1", 0, 450), i3_p1("i3_p1", 0, 450);
	input icomp00("icomp00", {i1_p1,i2_p1,i3_p1}, p_float64);
	computation comp00("comp00", {i0,i1,i2,i3},  p_float64);
	comp00.set_expression(((((icomp00(i1, i2, i3 - 1) + icomp00(i1, i2 + 1, i3 + 1) + icomp00(i1 - 1, i2 - 1, i3) + icomp00(i1 - 1, i2 - 1, i3 - 1) + icomp00(i1 - 1, i2 - 1, i3 + 1)*icomp00(i1 - 1, i2 + 1, i3 + 1) + icomp00(i1 - 1, i2 + 1, i3 - 1) + icomp00(i1 + 1, i2, i3 - 1) + icomp00(i1 + 1, i2 - 1, i3 - 1) + 7.430)*icomp00(i1 + 1, i2, i3 + 1) + expr(5.540)*icomp00(i1, i2, i3 + 1)*icomp00(i1, i2 - 1, i3)*icomp00(i1 + 1, i2 + 1, i3) - icomp00(i1, i2 - 1, i3 + 1)*icomp00(i1 - 1, i2, i3 - 1) + icomp00(i1 - 1, i2, i3) + icomp00(i1 - 1, i2, i3 + 1) - icomp00(i1 + 1, i2, i3) - icomp00(i1 + 1, i2 - 1, i3) + icomp00(i1 + 1, i2 + 1, i3 - 1))*icomp00(i1 - 1, i2 + 1, i3) + icomp00(i1, i2, i3) - icomp00(i1, i2 + 1, i3 - 1) + icomp00(i1 + 1, i2 - 1, i3 + 1))*icomp00(i1, i2 + 1, i3) + icomp00(i1, i2 - 1, i3 - 1)*icomp00(i1 + 1, i2 + 1, i3 + 1))/icomp00(i1, i2 + 1, i3));
	buffer buf00("buf00", {66,450,450}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00, {i1,i2,i3});

	prepare_schedules_for_legality_checks();
	perform_full_dependency_analysis();

	const int beam_size = get_beam_size();
	const int max_depth = get_max_depth();
	declare_memory_usage();

	auto_scheduler::schedules_generator *scheds_gen = new auto_scheduler::ml_model_schedules_generator();
	auto_scheduler::evaluate_by_execution *exec_eval = new auto_scheduler::evaluate_by_execution({&buf00}, "function7522636.o", "./function7522636_wrapper");
	auto_scheduler::search_method *bs = new auto_scheduler::beam_search(beam_size, max_depth, exec_eval, scheds_gen);
	auto_scheduler::auto_scheduler as(bs, exec_eval);
	as.set_exec_evaluator(exec_eval);
	as.sample_search_space("./function7522636_explored_schedules.json", true);
	delete scheds_gen;
	delete exec_eval;
	delete bs;
	return 0;
}