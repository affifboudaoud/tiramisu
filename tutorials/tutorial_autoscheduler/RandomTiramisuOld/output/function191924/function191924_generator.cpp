#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191924_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191924");
	var i0("i0", 1, 65), i1("i1", 0, 32), i0_p1("i0_p1", 0, 66), i0_p0("i0_p0", 0, 65);
	input icomp00("icomp00", {i0_p1}, p_float64);
	input input01("input01", {i0_p0,i1}, p_float64);
	input input02("input02", {i0_p0,i1}, p_float64);
	computation comp00("comp00", {i0,i1},  p_float64);
	comp00.set_expression(((icomp00(i0) - icomp00(i0 + 1) + input02(i0, i1))*icomp00(i0 - 1) + expr(4.460)*input01(i0, i1))/(icomp00(i0) - icomp00(i0 + 1) + input02(i0, i1)));
	buffer buf00("buf00", {66}, p_float64, a_output);
	buffer buf01("buf01", {65,32}, p_float64, a_input);
	buffer buf02("buf02", {65,32}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	input02.store_in(&buf02);
	comp00.store_in(&buf00, {i0});
	tiramisu::codegen({&buf00,&buf01,&buf02}, "function191924.o"); 
	return 0; 
}