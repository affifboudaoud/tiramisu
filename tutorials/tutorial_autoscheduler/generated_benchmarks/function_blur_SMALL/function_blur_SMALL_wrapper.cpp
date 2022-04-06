#include "Halide.h"
#include "function_blur_SMALL_wrapper.h"
#include "tiramisu/utils.h"
#include <iostream>
#include <time.h>
#include <fstream>
#include <chrono>

using namespace std::chrono;
using namespace std;      

int main(int, char **argv)
{
    double *c_b_in = (double*)malloc(98*34*5*sizeof(double));
	parallel_init_buffer(c_b_in,98*34*5, (double)45);
	Halide::Buffer<double> b_in(c_b_in, 98,34,5);

	double *c_b_out = (double*)malloc(98*34*5*sizeof(double));
	parallel_init_buffer(c_b_out,98*34*5, (double)19);
	Halide::Buffer<double> b_out(c_b_out, 98,34,5);

	int nb_exec = get_nb_exec();

	for (int i = 0; i < nb_exec; i++) 
	{  
		auto begin = std::chrono::high_resolution_clock::now(); 
		function_blur_SMALL(b_in.raw_buffer(),b_out.raw_buffer());
		auto end = std::chrono::high_resolution_clock::now(); 

		std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count() / (double)1000000 << " " << std::flush; 
	}
		std::cout << std::endl;

	return 0; 
}