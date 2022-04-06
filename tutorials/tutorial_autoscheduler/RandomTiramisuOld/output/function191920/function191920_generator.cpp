#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191920_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191920");
	var i0("i0", 1, 2049), i1("i1", 1, 257), i2("i2", 0, 256), i0_p1("i0_p1", 0, 2050), i1_p1("i1_p1", 0, 258);
	input icomp00("icomp00", {i0_p1,i1_p1}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(((icomp00(i0, i1) - icomp00(i0 + 1, i1))*(icomp00(i0, i1 - 1) + icomp00(i0, i1 + 1) + icomp00(i0 - 1, i1 - 1) + icomp00(i0 - 1, i1 + 1) + icomp00(i0 + 1, i1 - 1) + icomp00(i0 + 1, i1 + 1) + 1.220) + icomp00(i0 - 1, i1))/(icomp00(i0, i1) - icomp00(i0 + 1, i1)));
	buffer buf00("buf00", {2050,258}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00, {i0,i1});
	tiramisu::codegen({&buf00}, "function191920.o"); 
	return 0; 
}