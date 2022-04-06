#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191922_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191922");
	var i0("i0", 1, 33), i1("i1", 1, 65), i2("i2", 1, 33), i0_p1("i0_p1", 0, 34), i1_p1("i1_p1", 0, 66), i2_p1("i2_p1", 0, 34);
	input icomp00("icomp00", {i0_p1,i1_p1,i2_p1}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression((((icomp00(i0, i1, i2 + 1) + icomp00(i0 - 1, i1, i2)*icomp00(i0 - 1, i1 - 1, i2) + icomp00(i0 + 1, i1, i2))*icomp00(i0 - 1, i1 + 1, i2 - 1) + (icomp00(i0, i1, i2) + icomp00(i0, i1 - 1, i2 - 1) + icomp00(i0 + 1, i1 - 1, i2) + icomp00(i0 + 1, i1 + 1, i2 + 1) - 2.490)*icomp00(i0, i1 + 1, i2 - 1) + icomp00(i0, i1, i2 - 1)*icomp00(i0, i1 - 1, i2 + 1)*icomp00(i0 - 1, i1 - 1, i2 + 1)*icomp00(i0 + 1, i1, i2 + 1) + icomp00(i0, i1 + 1, i2) + icomp00(i0 - 1, i1, i2 - 1) - icomp00(i0 - 1, i1, i2 + 1) - icomp00(i0 - 1, i1 + 1, i2) + icomp00(i0 - 1, i1 + 1, i2 + 1) - icomp00(i0 + 1, i1, i2 - 1) + icomp00(i0 + 1, i1 - 1, i2 - 1) + icomp00(i0 + 1, i1 - 1, i2 + 1) + icomp00(i0 + 1, i1 + 1, i2) + icomp00(i0 + 1, i1 + 1, i2 - 1))*icomp00(i0, i1 - 1, i2) + icomp00(i0, i1 + 1, i2 + 1)*icomp00(i0 - 1, i1 - 1, i2 - 1))/icomp00(i0, i1 - 1, i2));
	buffer buf00("buf00", {34,66,34}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00);
	tiramisu::codegen({&buf00}, "function191922.o"); 
	return 0; 
}