#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_matmul_SMALL_wrapper.h"

using namespace tiramisu;


int main(int argc, char **argv)
{
    tiramisu::init("function_matmul_SMALL");

    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 
    var l("l", 0, 64), m("m", 0, 128), n("n", 0, 96);
    //inputs
    input A("A", {l, m}, p_float64);
    input B("B", {m, n}, p_float64);
    
    //Computations
    computation comp_matmul("comp_matmul", {l, n, m}, p_float64);
    comp_matmul.set_expression(comp_matmul(l, n, m) + A(l, m)*B(m, n));

    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------
    
    
    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------
    //Input Buffers
    buffer buf_A("A", {64, 128}, p_float64, a_input);
    buffer buf_B("B", { 128, 96}, p_float64, a_input);
    buffer buf_matmul("buf_matmul", {64, 96}, p_float64, a_output);
    
    //Store inputs
    A.store_in(&buf_A);
    B.store_in(&buf_B);

    //Store computations
    comp_matmul.store_in(&buf_matmul, {l, n});
   

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------
    tiramisu::codegen({&buf_A, &buf_B, &buf_matmul}, "function_matmul_SMALL.o");
    
    return 0;
}