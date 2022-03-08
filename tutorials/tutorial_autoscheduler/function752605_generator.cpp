#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function752605_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function752605");
	var i0("i0", 0, 32), i1("i1", 1, 33), i2("i2", 0, 32), i1_p1("i1_p1", 0, 34), i1_p0("i1_p0", 0, 33);
	input icomp00("icomp00", {i0,i1_p1}, p_float64);
	input input01("input01", {i2,i1_p0,i0}, p_float64);
	input input02("input02", {i0,i1_p0,i2}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(icomp00(i0, i1)*icomp00(i0, i1 - 1) + icomp00(i0, i1 + 1) + input01(i2, i1, i0) + input02(i0, i1, i2));
	buffer buf00("buf00", {32,34}, p_float64, a_output);
	buffer buf01("buf01", {32,33,32}, p_float64, a_input);
	buffer buf02("buf02", {32,33,32}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	input02.store_in(&buf02);
	comp00.store_in(&buf00, {i0,i1});
	tiramisu::codegen({&buf00,&buf01,&buf02}, "function752605.o"); 
	return 0; 
}