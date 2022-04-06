#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function750312_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function750312");
	var i0("i0", 0, 32), i1("i1", 0, 64), i2("i2", 0, 64), i3("i3", 0, 64);
	input icomp00("icomp00", {i1,i2,i3}, p_float64);
	input input01("input01", {i0,i1,i2}, p_float64);
	input input02("input02", {i1,i2,i3}, p_float64);
	computation comp00("comp00", {i0,i1,i2,i3},  p_float64);
	comp00.set_expression(icomp00(i1, i2, i3)*input02(i1, i2, i3) - input01(i0, i1, i2));
	buffer buf00("buf00", {64,64,64}, p_float64, a_output);
	buffer buf01("buf01", {32,64,64}, p_float64, a_input);
	buffer buf02("buf02", {64,64,64}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	input02.store_in(&buf02);
	comp00.store_in(&buf00, {i1,i2,i3});
	tiramisu::codegen({&buf00,&buf01,&buf02}, "function750312.o"); 
	return 0; 
}