#include "Halide.h"
#include "function_heat2d_XLARGE_wrapper.h"
#include "tiramisu/utils.h"
#include <iostream>
#include <time.h>
#include <fstream>
#include <chrono>

using namespace std::chrono;
using namespace std;      

int main(int, char **argv)
{
    double *c_b_in = (double*)malloc(3074*2562*sizeof(double));
	parallel_init_buffer(c_b_in,3074*2562, (double)45);
	Halide::Buffer<double> b_in(c_b_in, 3074,2562);

	double *c_b_out = (double*)malloc(3074*2562*sizeof(double));
	parallel_init_buffer(c_b_out,3074*2562, (double)19);
	Halide::Buffer<double> b_out(c_b_out, 3074,2562);

	int nb_exec = get_nb_exec();

	for (int i = 0; i < nb_exec; i++) 
	{  
		auto begin = std::chrono::high_resolution_clock::now(); 
		function_heat2d_XLARGE(b_in.raw_buffer(),b_out.raw_buffer());
		auto end = std::chrono::high_resolution_clock::now(); 

		std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count() / (double)1000000 << " " << std::flush; 
	}
		std::cout << std::endl;

	return 0; 
}