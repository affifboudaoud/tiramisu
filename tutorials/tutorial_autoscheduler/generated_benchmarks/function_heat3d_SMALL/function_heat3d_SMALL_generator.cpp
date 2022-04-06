#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_heat3d_SMALL_wrapper.h"

using namespace tiramisu;

int main(int argc, char **argv)
{
    tiramisu::init("function_heat3d_SMALL");

    // -------------------------------------------------------
    // Layer I
    // -------------------------------------------------------     
    //for heat3d_init
    var x_in("x_in", 0, 34), y_in("y_in", 0, 34), z_in("z_in", 0, 34), t_in("t_in", 0, 96);
    //for heat3d_c
    var x("x",1,34-1), y("y",1,34-1), z("z",1,34-1), t("t",0,96);

    computation comp_heat3d("heat3d",{t,z,y,x},p_float64);
    comp_heat3d.set_expression(
		comp_heat3d(t,z,y,x) +
		    (comp_heat3d(t,z-1,y,x) - comp_heat3d(t,z,y,x)*2.0 + comp_heat3d(t,z+1,y,x)
			 + comp_heat3d(t,z,y-1,x) - comp_heat3d(t,z,y,x)*2.0 + comp_heat3d(t,z,y+1,x)
			 + comp_heat3d(t,z,y,x-1) - comp_heat3d(t,z,y,x)*2.0 + comp_heat3d(t,z,y,x+1))*0.125);
  
    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------    

    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------    
    //buffers
    buffer b_out("b_out",{34,34,34},p_float64,a_output);
    
    //Store inputs

    //Store computations  
    comp_heat3d.store_in(&b_out,{z,y,x});

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------
    tiramisu::codegen({&b_out}, "./function_heat3d_SMALL.o");
    return 0;
}