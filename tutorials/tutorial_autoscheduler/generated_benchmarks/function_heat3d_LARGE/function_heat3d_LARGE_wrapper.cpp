#include "Halide.h"
#include "function_heat3d_LARGE_wrapper.h"
#include "tiramisu/utils.h"
#include <iostream>
#include <time.h>
#include <fstream>
#include <chrono>

using namespace std::chrono;
using namespace std;      

int main(int, char **argv)
{

	double *c_b_out = (double*)malloc(130*130*130*sizeof(double));
	parallel_init_buffer(c_b_out,130*130*130, (double)19);
	Halide::Buffer<double> b_out(c_b_out, 130,130,130);

	int nb_exec = get_nb_exec();

	for (int i = 0; i < nb_exec; i++) 
	{  
		auto begin = std::chrono::high_resolution_clock::now(); 
		function_heat3d_LARGE(b_out.raw_buffer());
		auto end = std::chrono::high_resolution_clock::now(); 

		std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count() / (double)1000000 << " " << std::flush; 
	}
		std::cout << std::endl;

	return 0; 
}