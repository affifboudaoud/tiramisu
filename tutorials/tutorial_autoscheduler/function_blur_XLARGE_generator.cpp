#include <tiramisu/tiramisu.h>
#include <tiramisu/auto_scheduler/evaluator.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include "function_blur_XLARGE_wrapper.h"


using namespace tiramisu;

int main(int argc, char **argv)
{
    tiramisu::init("function_blur_XLARGE");

    // -------------------------------------------------------
    // Layer I
    // ------------------------------------------------------- 
    var xi("xi", 0, 2562), yi("yi", 0, 1026), ci("ci", 0, 5);
    var x("x", 1, 2562-1), y("y", 1, 1026-1), c("c", 1, 5-1);

    //inputs
    input input_img("input_img", {ci, yi, xi}, p_float64);

    //Computations
    computation comp_blur("comp_blur", {c, y, x}, (input_img(c, y + 1, x - 1) + input_img(c, y + 1, x) + input_img(c, y + 1, x + 1) + 
                                         input_img(c, y, x - 1)     + input_img(c, y, x)     + input_img(c, y, x + 1) + 
                                         input_img(c, y - 1, x - 1) + input_img(c, y - 1, x) + input_img(c, y - 1, x + 1))*0.111111);
    
    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------


    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------
    //Buffers
    buffer input_buf("input_buf", {5, 1026, 2562}, p_float64, a_input);
    buffer output_buf("output_buf", {5,1026, 2562}, p_float64, a_output);

    //Store inputs
    input_img.store_in(&input_buf);

    //Store computations
    comp_blur.store_in(&output_buf);
 
    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------
    tiramisu::codegen({&input_buf, &output_buf}, "./function_blur_XLARGE.o");

    return 0;
}