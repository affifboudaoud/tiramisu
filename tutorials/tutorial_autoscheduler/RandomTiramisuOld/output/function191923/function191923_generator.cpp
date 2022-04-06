#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191923_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191923");
	var i0("i0", 0, 64), i1("i1", 0, 64), i2("i2", 0, 64), i3("i3", 1, 129), i4("i4", 1, 385), i5("i5", 1, 385), i3_p1("i3_p1", 0, 130), i4_p1("i4_p1", 0, 386), i5_p1("i5_p1", 0, 386);
	input icomp00("icomp00", {i3_p1,i4_p1,i5_p1}, p_float64);
	computation comp00("comp00", {i0,i1,i2,i3,i4,i5},  p_float64);
	comp00.set_expression(icomp00(i3, i4, i5) + expr(0.900)*icomp00(i3, i4, i5 - 1) + icomp00(i3, i4, i5 + 1)*icomp00(i3, i4 - 1, i5)*icomp00(i3, i4 + 1, i5)*icomp00(i3 + 1, i4, i5) + icomp00(i3 - 1, i4, i5));
	buffer buf00("buf00", {130,386,386}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00, {i3,i4,i5});
	tiramisu::codegen({&buf00}, "function191923.o"); 
	return 0; 
}