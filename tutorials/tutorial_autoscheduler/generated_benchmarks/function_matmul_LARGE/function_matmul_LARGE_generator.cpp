#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_matmul_LARGE_wrapper.h"

using namespace tiramisu;


int main(int argc, char **argv)
{
    tiramisu::init("function_matmul_LARGE");

    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 
    var l("l", 0, 1024), m("m", 0, 1536), n("n", 0, 1280);
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
    buffer buf_A("A", {1024, 1536}, p_float64, a_input);
    buffer buf_B("B", { 1536, 1280}, p_float64, a_input);
    buffer buf_matmul("buf_matmul", {1024, 1280}, p_float64, a_output);
    
    //Store inputs
    A.store_in(&buf_A);
    B.store_in(&buf_B);

    //Store computations
    comp_matmul.store_in(&buf_matmul, {l, n});
   

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------
    tiramisu::codegen({&buf_A, &buf_B, &buf_matmul}, "function_matmul_LARGE.o");
    
    return 0;
}