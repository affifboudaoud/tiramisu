#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function000007_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function000007");
	var i0("i0", 1, 129), i1("i1", 0, 448), i2("i2", 0, 64), i0_p0("i0_p0", 0, 129);
	input icomp00("icomp00", {i0_p0,i1,i2}, p_float64);
	input input01("input01", {i0_p0}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(icomp00(i0, i1, i2) + input01(i0 - 1));
	buffer buf00("buf00", {129,448,64}, p_float64, a_output);
	buffer buf01("buf01", {129}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	comp00.store_in(&buf00);
	tiramisu::codegen({&buf00,&buf01}, "function000007.o"); 
	return 0; 
}