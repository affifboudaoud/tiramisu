#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function7522636_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function7522636");
	var i0("i0", 0, 128), i1("i1", 1, 65), i2("i2", 1, 449), i3("i3", 1, 449), i1_p1("i1_p1", 0, 66), i2_p1("i2_p1", 0, 450), i3_p1("i3_p1", 0, 450);
	input icomp00("icomp00", {i1_p1,i2_p1,i3_p1}, p_float64);
	computation comp00("comp00", {i0,i1,i2,i3},  p_float64);
	comp00.set_expression(((((icomp00(i1, i2, i3 - 1) + icomp00(i1, i2 + 1, i3 + 1) + icomp00(i1 - 1, i2 - 1, i3) + icomp00(i1 - 1, i2 - 1, i3 - 1) + icomp00(i1 - 1, i2 - 1, i3 + 1)*icomp00(i1 - 1, i2 + 1, i3 + 1) + icomp00(i1 - 1, i2 + 1, i3 - 1) + icomp00(i1 + 1, i2, i3 - 1) + icomp00(i1 + 1, i2 - 1, i3 - 1) + 7.430)*icomp00(i1 + 1, i2, i3 + 1) + expr(5.540)*icomp00(i1, i2, i3 + 1)*icomp00(i1, i2 - 1, i3)*icomp00(i1 + 1, i2 + 1, i3) - icomp00(i1, i2 - 1, i3 + 1)*icomp00(i1 - 1, i2, i3 - 1) + icomp00(i1 - 1, i2, i3) + icomp00(i1 - 1, i2, i3 + 1) - icomp00(i1 + 1, i2, i3) - icomp00(i1 + 1, i2 - 1, i3) + icomp00(i1 + 1, i2 + 1, i3 - 1))*icomp00(i1 - 1, i2 + 1, i3) + icomp00(i1, i2, i3) - icomp00(i1, i2 + 1, i3 - 1) + icomp00(i1 + 1, i2 - 1, i3 + 1))*icomp00(i1, i2 + 1, i3) + icomp00(i1, i2 - 1, i3 - 1)*icomp00(i1 + 1, i2 + 1, i3 + 1))/icomp00(i1, i2 + 1, i3));
	buffer buf00("buf00", {66,450,450}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00, {i1,i2,i3});
	tiramisu::codegen({&buf00}, "function7522636.o"); 
	return 0; 
}