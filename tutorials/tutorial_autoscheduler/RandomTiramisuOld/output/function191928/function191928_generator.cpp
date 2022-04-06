#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191928_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191928");
	var i0("i0", 0, 64), i1("i1", 0, 192), i2("i2", 0, 128);
	input icomp00("icomp00", {i0,i1,i2}, p_float64);
	computation comp00("comp00", {i0,i1,i2}, 1);
	buffer buf00("buf00", {64,192,128}, p_float64, a_output);
	icomp00.store_in(&buf00);
	comp00.store_in(&buf00);
	tiramisu::codegen({&buf00}, "function191928.o"); 
	return 0; 
}