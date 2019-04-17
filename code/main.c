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
The matrices are read from files with the name of the blocks(Hcc,...).
*/

#include<stdio.h>
#include<mpi.h>

#define CAM_PARAMS 353 		
#define STRUCT_PARAMS 78963 

//driver function
int main()
{

	return 0;
}