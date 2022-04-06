#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function000008_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function000008");
	var i0("i0", 0, 32), i1("i1", 1, 33), i2("i2", 0, 64), i1_p1("i1_p1", 0, 34);
	input icomp00("icomp00", {i1_p1,i2}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(icomp00(i1, i2) + icomp00(i1 - 1, i2) + icomp00(i1 + 1, i2));
	buffer buf00("buf00", {34,64}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00, {i1,i2});
	tiramisu::codegen({&buf00}, "function000008.o"); 
	return 0; 
}