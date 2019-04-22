/*
The data for this domain decomposition comes from the Bundle Adjustment problem.
The coefficient matrix is the Hessian obtained from the Levenberg-Marquardt algorithm.
The RHS is also obtained from the same problem. The matrix has an arrowhead structure that
can be partitioned into blocks as follows:
			
				| Hcc	Hsc |
			A = |			|
				| Hsc   Hss |
Here,
	A is symmetric,
	Hsc = Hcs',
	Hcc is the reduced camera matrix,
	Hss is the reduced structure matrix.
In the code below, CAM_PARAMS refer to the EO and IO camera parameters and
STRUCT_PARAMS refer to the no of structure parameters which is 3 x no of object points.
The matrices are read from files with the name of the blocks(Hcc,...). In these files, the 
first entry gives the no of non zeros in the matrix block.

Compile as : cc -w main.c st_io.c
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>

#include"st_io.h"
#include"read_coo_matrix.h"
#include"read_b.h"


#define CAM_PARAMS 353 		
#define STRUCT_PARAMS 21000




//driver function
int main()
{
	coo_mat* Hcc;
	coo_mat* Hcs;
	coo_mat* Hsc;
	coo_mat* Hss;
	double*  b;

	Hcc = read_coo_matrix("Hcc",CAM_PARAMS,CAM_PARAMS);
	Hcs = read_coo_matrix("Hcs",CAM_PARAMS,STRUCT_PARAMS);
	Hsc = read_coo_matrix("Hsc",STRUCT_PARAMS,CAM_PARAMS);
	Hss = read_coo_matrix("Hss",STRUCT_PARAMS,STRUCT_PARAMS);
	b = read_b(CAM_PARAMS+STRUCT_PARAMS); // read the RHS of the system

	
	printf("\nHcc_row -> %d\n", Hcc->row_idx[8]); // -1.4099e+07
	printf("\nHcc_col -> %d\n", Hcc->col_idx[8]); // -2139.91
	//printf("\nHsc -> %lf\n", Hsc[2][0]); // -2139.91
	printf("\nHcc_val -> %lf\n", Hcc->val[8]); // 6479.53
	printf("\nb -> %lf\n", b[2]); // -4.07038e+06

	return 0;
}