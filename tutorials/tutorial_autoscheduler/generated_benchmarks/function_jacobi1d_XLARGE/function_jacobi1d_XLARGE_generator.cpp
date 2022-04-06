#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_jacobi1d_XLARGE_wrapper.h"

using namespace tiramisu;


int main(int argc, char **argv)
{
    tiramisu::init("function_jacobi1d_XLARGE");

    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 

    //Iteration variables    
    var i_f("i_f", 0, 4098);
    var t("t", 0, 2048), i("i", 1, 4098-1);
    
    //inputs
    input A("A", {i_f}, p_float64);


    //Computations
    computation A_out("A_out", {t,i}, (A(i-1) + A(i) + A(i + 1))*0.33333);

    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------
    
    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------
    //Input Buffers
    buffer b_A("b_A", {4098}, p_float64, a_output);    

    //Store inputs
    A.store_in(&b_A);

    //Store computations
    A_out.store_in(&b_A, {i});

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------
    tiramisu::codegen({&b_A}, "function_jacobi1d_XLARGE.o");

    return 0;
}