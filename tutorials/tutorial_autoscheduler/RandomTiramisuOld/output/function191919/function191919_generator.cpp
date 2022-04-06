#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191919_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191919");
	var i0("i0", 0, 320), i1("i1", 0, 64), i2("i2", 0, 64);
	input icomp00("icomp00", {i1,i2}, p_float64);
	input input01("input01", {i0,i1,i2}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(icomp00(i1, i2) + input01(i0, i1, i2));
	buffer buf00("buf00", {64,64}, p_float64, a_output);
	buffer buf01("buf01", {320,64,64}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	comp00.store_in(&buf00, {i1,i2});
	tiramisu::codegen({&buf00,&buf01}, "function191919.o"); 
	return 0; 
}