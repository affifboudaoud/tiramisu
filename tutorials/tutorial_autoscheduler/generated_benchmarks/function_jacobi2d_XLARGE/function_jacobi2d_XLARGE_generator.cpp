#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_jacobi2d_XLARGE_wrapper.h"

using namespace tiramisu;


int main(int argc, char **argv)
{
    tiramisu::init("function_jacobi2d_XLARGE");

    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 

    //Iteration variables    
    var i_f("i_f", 0, 4098), j_f("j_f", 0, 4098);
    var t("t", 0, 2048), i("i", 1, 4098-1), j("j", 1, 4098-1);
    
    //inputs
    input A("A", {i_f, j_f}, p_float64);

    //Computations
    computation comp_A_out("comp_A_out", {t,i,j}, (A(i, j) + A(i, j-1) + A(i, 1+j) + A(1+i, j) + A(i-1, j))*0.2);

    //Input Buffers
    buffer b_A("b_A", {4098,4098}, p_float64, a_output);    

    //Store inputs
    A.store_in(&b_A);


    //Store computations
    comp_A_out.store_in(&b_A, {i,j});

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------
    tiramisu::codegen({&b_A}, "function_jacobi2d_XLARGE.o");

    return 0;
}