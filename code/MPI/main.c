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

The no of processors this can be run on is 3
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>
#include<mpi.h>

#include"st_io.h"
#include"read_coo_matrix.h"
#include"read_b.h"
//#include"compute_block_inverse.h"

//parameters for the ROMA dataset
#define CAM_PARAMS 353 		
#define STRUCT_PARAMS 78963




//driver function
int main()
{
	int rank,size,new_rank;

	// proc ranks for new group
	int new_ranks[3] = {0,1,2};

	//master proc
	int root = 0; 

	// group handles for original and new group 
	MPI_Group orig_group, new_group; 
	MPI_Comm new_comm;

	coo_mat* Hcc;
	coo_mat* Hcs;
	coo_mat* Hsc;
	coo_mat* Hss;
	coo_mat* Hss_inv;
	double*  b;
	int      Hss_block;
	int*     recv_row;
	int*     recv_col;
	double*  recv_val;
	

	MPI_Init(0,0);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	if(size < 3)
	{
		printf("\nNo of procs must be atleast 3.\n");
		MPI_Finalize();
		exit(0);
	}

	//extracting original group handle
	MPI_Comm_group(MPI_COMM_WORLD,&orig_group);

	//create new group
	MPI_Group_incl(orig_group, 3, new_ranks, &new_group);

	//create new communicator
	MPI_Comm_create(MPI_COMM_WORLD, new_group, &new_comm);

	//get rank of each process in a group
	MPI_Group_rank(new_group,&new_rank);


	//block size of Hss to be sent to each processor
	//since Hss blocks are 3x3 blocks
	Hss_block = STRUCT_PARAMS/3; 

	//allocating memory to the receiving arrays
	recv_row = (int*) malloc (Hss_block*sizeof(int));
	recv_col = (int*) malloc (Hss_block*sizeof(int));
	recv_val = (double*) malloc (Hss_block*sizeof(double));


	//read the matrices in the master process.
	if(rank == root)
	{
		printf("\n==============Reading Matrices=================\n");

		Hcc = read_coo_matrix("Hcc",CAM_PARAMS,CAM_PARAMS);
		Hcs = read_coo_matrix("Hcs",CAM_PARAMS,STRUCT_PARAMS);
		Hsc = read_coo_matrix("Hsc",STRUCT_PARAMS,CAM_PARAMS);
		Hss = read_coo_matrix("Hss",STRUCT_PARAMS,STRUCT_PARAMS);

		// read the RHS of the system
		b = read_b(CAM_PARAMS+STRUCT_PARAMS); 

		//printf("\nIn Hss, nnz = %d\n",Hss->nnz); // 236889 = 3 x 78963

		//printf("\nHss_block : %d\n", Hss_block);

		MPI_Scatter(Hss->row_idx, Hss_block, MPI_INT, recv_row, Hss_block, MPI_INT, root, new_comm);
		MPI_Scatter(Hss->col_idx, Hss_block, MPI_INT, recv_col, Hss_block, MPI_INT, root, new_comm);
		MPI_Scatter(Hss->val, Hss_block, MPI_DOUBLE, recv_val, Hss_block, MPI_DOUBLE, root, new_comm);

		printf("\nIn rank %d ,Hss_block : %d\n", rank,Hss_block);
	}	
	else if(rank == 1 || rank == 2)
	{
		MPI_Scatter(NULL, 0, MPI_INT, recv_row, Hss_block, MPI_INT, root, new_comm);
		MPI_Scatter(NULL, 0, MPI_INT, recv_col, Hss_block, MPI_INT, root, new_comm);
		MPI_Scatter(NULL, 0, MPI_DOUBLE, recv_val, Hss_block, MPI_DOUBLE, root, new_comm);

		printf("\nIn rank %d ,Hss_block : %d\n", rank,Hss_block);
	}

	//synchronize
	MPI_Barrier(MPI_COMM_WORLD);
	
	MPI_Finalize();

	return 0;
}