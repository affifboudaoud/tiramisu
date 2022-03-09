#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function000005_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function000005");
	var i0("i0", 0, 256), i1("i1", 1, 1281), i2("i2", 1, 1025), i1_p1("i1_p1", 0, 1282), i2_p1("i2_p1", 0, 1026);
	input icomp00("icomp00", {i1_p1,i2_p1}, p_float64);
	computation comp00("comp00", {i0,i1,i2},  p_float64);
	comp00.set_expression(expr(0.0) - expr(1.860)*icomp00(i1, i2)*icomp00(i1 + 1, i2) + icomp00(i1, i2) + icomp00(i1, i2 - 1) - icomp00(i1, i2 + 1) + icomp00(i1 - 1, i2));
	buffer buf00("buf00", {1282,1026}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00, {i1,i2});
	tiramisu::codegen({&buf00}, "function000005.o"); 
	return 0; 
}