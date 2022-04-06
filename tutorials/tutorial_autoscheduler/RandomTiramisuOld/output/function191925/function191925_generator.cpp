#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function191925_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function191925");
	var i0("i0", 0, 128), i1("i1", 0, 96);
	computation comp00("comp00", {i0,i1}, 1.340);
	buffer buf00("buf00", {128,96}, p_float64, a_output);
	comp00.store_in(&buf00);
	tiramisu::codegen({&buf00}, "function191925.o"); 
	return 0; 
}