#include <tiramisu/tiramisu.h> 
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function750483_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv){                
	tiramisu::init("function750483");
	var i0("i0", 0, 64), i1("i1", 0, 32), i2("i2", 1, 65), i3("i3", 1, 33), i2_p1("i2_p1", 0, 66), i3_p1("i3_p1", 0, 34), i2_p0("i2_p0", 0, 65), i3_p0("i3_p0", 0, 33);
	input input01("input01", {i2_p1,i3_p1}, p_float64);
	input input02("input02", {i0,i1,i2_p0,i3_p0}, p_float64);
	computation comp00("comp00", {i0,i1,i2,i3}, ((-(input01(i2, i3 - 1) + 1.150)*input01(i2 + 1, i3)*input01(i2 + 1, i3 + 1) + input01(i2 - 1, i3) + input01(i2 - 1, i3 + 1) + input01(i2 + 1, i3 - 1)*input02(i0, i1, i2, i3))*input01(i2 - 1, i3 - 1) + input01(i2, i3)*input01(i2, i3 + 1))/input01(i2 - 1, i3 - 1));
	buffer buf00("buf00", {64,32,65,33}, p_float64, a_output);
	buffer buf01("buf01", {66,34}, p_float64, a_input);
	buffer buf02("buf02", {64,32,65,33}, p_float64, a_input);
	input01.store_in(&buf01);
	input02.store_in(&buf02);
	comp00.store_in(&buf00);
	tiramisu::codegen({&buf00,&buf01,&buf02}, "function750483.o"); 
	return 0; 
}