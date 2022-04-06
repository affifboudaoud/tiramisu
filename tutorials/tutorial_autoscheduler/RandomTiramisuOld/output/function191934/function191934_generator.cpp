#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191934_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191934");
	var i0("i0", 1, 33), i1("i1", 1, 33), i2("i2", 0, 128), i0_p1("i0_p1", 0, 34), i1_p1("i1_p1", 0, 34);
	input icomp00("icomp00", {i0_p1,i1_p1}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(icomp00(i0, i1)*icomp00(i0, i1 - 1) + icomp00(i0, i1 + 1)*icomp00(i0 - 1, i1) - icomp00(i0 + 1, i1));
	buffer buf00("buf00", {34,34}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00, {i0,i1});
	tiramisu::codegen({&buf00}, "function191934.o"); 
	return 0; 
}