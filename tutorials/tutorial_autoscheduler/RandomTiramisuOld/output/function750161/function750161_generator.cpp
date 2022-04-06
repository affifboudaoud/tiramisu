#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function750161_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function750161");
	var i0("i0", 0, 64), i1("i1", 0, 64), i2("i2", 0, 96), i3("i3", 0, 96), i0_p1("i0_p1", 0, 65);
	input icomp00("icomp00", {i1,i2,i3}, p_float64);
	input input01("input01", {i0_p1,i1,i2}, p_float64);
	computation comp00("comp00", {i0,i1,i2,i3},  p_float64);
	comp00.set_expression(expr(1.240)*icomp00(i1, i2, i3) + expr(1.240)*input01(i0 + 1, i1, i2) - 5.816);
	buffer buf00("buf00", {64,96,96}, p_float64, a_output);
	buffer buf01("buf01", {65,64,96}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	comp00.store_in(&buf00, {i1,i2,i3});
	tiramisu::codegen({&buf00,&buf01}, "function750161.o"); 
	return 0; 
}