#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191931_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191931");
	var i0("i0", 0, 2048), i1("i1", 0, 2048), i2("i2", 0, 2048);
	input icomp00("icomp00", {i0,i1}, p_float64);
	input input01("input01", {i1,i2}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(icomp00(i0, i1) + input01(i1, i2));
	buffer buf00("buf00", {2048,2048}, p_float64, a_output);
	buffer buf01("buf01", {2048,2048}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	comp00.store_in(&buf00, {i0,i1});
	tiramisu::codegen({&buf00,&buf01}, "function191931.o"); 
	return 0; 
}