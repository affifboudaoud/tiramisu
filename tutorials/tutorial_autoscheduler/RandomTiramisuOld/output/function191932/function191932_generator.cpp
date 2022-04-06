#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191932_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191932");
	var i0("i0", 0, 64), i1("i1", 0, 64), i2("i2", 0, 64);
	input icomp00("icomp00", {i1,i2}, p_float64);
	input input01("input01", {i2}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(expr(0.0) - icomp00(i1, i2) + input01(i2) + 3.310);
	buffer buf00("buf00", {64,64}, p_float64, a_output);
	buffer buf01("buf01", {64}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	comp00.store_in(&buf00, {i1,i2});
	tiramisu::codegen({&buf00,&buf01}, "function191932.o"); 
	return 0; 
}