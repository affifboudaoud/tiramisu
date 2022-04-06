#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_heat2d_XLARGE_wrapper.h"


using namespace tiramisu;

int main(int argc, char **argv)
{
    tiramisu::init("function_heat2d_XLARGE");
    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 
    var y_in("y_in", 0, 2562), x_in("x_in", 0, 3074), x("x", 1, 3074 - 1), y("y", 1, 2562 - 1);

    //Inputs
    input input("input", {y_in, x_in}, p_float64);

    //Computations
    computation comp_heat2d("comp_heat2d", {y, x}, p_float64);
    comp_heat2d.set_expression( input(y, x)*0.15 + (input(y, x + 1) + input(y, x -1) + input(y + 1, x) + input(y - 1, x))*0.2);

    buffer buff_input("buff_input", {2562, 3074}, p_float64, a_input);
    buffer buff_heat2d("buff_heat2d", {2562, 3074}, p_float64, a_output);

    //Store inputs
    input.store_in(&buff_input);

    //Store computations
    comp_heat2d.store_in(&buff_heat2d);

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------  
    tiramisu::codegen({&buff_input, &buff_heat2d}, "./function_heat2d_XLARGE.o");

    return 0;
}