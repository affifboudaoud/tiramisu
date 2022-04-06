#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191938_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191938");
	var i0("i0", 0, 64), i1("i1", 0, 96);
	input icomp00("icomp00", {i1}, p_float64);
	input input01("input01", {i0,i1}, p_float64);
	computation comp00("comp00", {i0,i1},  p_float64);
	comp00.set_expression(icomp00(i1) + expr(0.280)*input01(i0, i1));
	buffer buf00("buf00", {96}, p_float64, a_output);
	buffer buf01("buf01", {64,96}, p_float64, a_input);
	icomp00.store_in(&buf00);
	input01.store_in(&buf01);
	comp00.store_in(&buf00, {i1});
	tiramisu::codegen({&buf00,&buf01}, "function191938.o"); 
	return 0; 
}