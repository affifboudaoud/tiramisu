#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191935_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191935");
	var i0("i0", 1, 513), i1("i1", 1, 513), i2("i2", 0, 256), i0_p1("i0_p1", 0, 514), i1_p1("i1_p1", 0, 514);
	input icomp00("icomp00", {i0_p1,i1_p1}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression((expr(5.900)*(icomp00(i0, i1 - 1) + icomp00(i0 + 1, i1 + 1))*icomp00(i0, i1) + icomp00(i0 - 1, i1)*icomp00(i0 - 1, i1 - 1))*(icomp00(i0, i1 + 1) + icomp00(i0 + 1, i1)) + icomp00(i0 - 1, i1 + 1) + icomp00(i0 + 1, i1 - 1));
	buffer buf00("buf00", {514,514}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00, {i0,i1});
	tiramisu::codegen({&buf00}, "function191935.o"); 
	return 0; 
}