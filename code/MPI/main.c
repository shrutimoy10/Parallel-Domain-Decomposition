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
#include"block_operations.h"

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
	float*  b;
	int     Hss_nz_block[3];
	int* 	Hss_mat_block;
	int 	displs[3];
	//float** Hss_dense;
	

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
	/* we have 78963 / 3 = 26321 which is not divisible by 3.
	Hence, dividing the blocks into STRUCT_PARAMS/3 will not divide the non-zero
	arrays accurately. So divide the blocks appropriately such that the smaller 3x3 blocks 
	fit completely inside the matrix blocks.
	*/
	Hss_mat_block = generate_block_sizes(STRUCT_PARAMS); 
	
	coo_mat* Hss_recv = (coo_mat*) malloc(sizeof(coo_mat));


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

		Hss_nz_block[0] = Hss_mat_block[0]*3;
		Hss_nz_block[1] = Hss_mat_block[1]*3;
		Hss_nz_block[2] = Hss_mat_block[2]*3;

		printf("\nBlock 1: %d Block 2 : %d block 3: %d\n", Hss_mat_block[0], Hss_mat_block[1],Hss_mat_block[2]);
		printf("\nNon Zeros -> Block 1: %d Block 2 : %d block 3: %d\n", Hss_nz_block[0], Hss_nz_block[1],Hss_nz_block[2]);

		//allocating memory for the receiving buffers
		//allocation may fail for dense matrix
		Hss_recv->row_idx = (int*) malloc (Hss_nz_block[rank]*sizeof(int));
		Hss_recv->col_idx = (int*) malloc (Hss_nz_block[rank]*sizeof(int));
		Hss_recv->val = (float*) malloc (Hss_nz_block[rank]*sizeof(float));

		displs[0] = 0;
		displs[1] = Hss_nz_block[1];
		displs[2] = Hss_nz_block[2];

		MPI_Bcast(Hss_nz_block, 3, MPI_INT, root, new_comm);
		MPI_Scatterv(Hss->row_idx, Hss_nz_block, displs,MPI_INT, Hss_recv->row_idx, Hss_nz_block, MPI_INT, root, new_comm);
		MPI_Scatterv(Hss->col_idx, Hss_nz_block, displs,MPI_INT, Hss_recv->col_idx, Hss_nz_block, MPI_INT, root, new_comm);
		MPI_Scatterv(Hss->val, Hss_nz_block, displs,MPI_FLOAT, Hss_recv->val, Hss_nz_block, MPI_FLOAT, root, new_comm);
		/*
		printf("\nIn rank %d, Hss blk : %d\n",rank,Hss_nz_block[rank] );
		printf("\nIn rank %d ,Row : %d\n", rank,recv_row[9]);
		printf("\nIn rank %d ,Col : %d\n", rank,recv_col[9]);
		printf("\nIn rank %d ,Val : %f\n", rank,recv_val[9]);
		*/

		Hss_recv->nnz = Hss_nz_block[rank];


		//finding the block inverses of the matrix
		compute_block_inverse(Hss_recv,rank);

	}
	else if(rank == 1 || rank == 2)
	{
		MPI_Bcast(Hss_nz_block, 3, MPI_INT, root, new_comm);

		//allocating memory for the receiving buffers
		//allocation may fail for dense matrix
		Hss_recv->row_idx = (int*) malloc (Hss_nz_block[rank]*sizeof(int));
		Hss_recv->col_idx = (int*) malloc (Hss_nz_block[rank]*sizeof(int));
		Hss_recv->val = (float*) malloc (Hss_nz_block[rank]*sizeof(float));

		
		MPI_Scatterv(NULL, 0, NULL,MPI_INT, Hss_recv->row_idx, Hss_nz_block[rank], MPI_INT, root, new_comm);
		MPI_Scatterv(NULL, 0, NULL,MPI_INT, Hss_recv->col_idx, Hss_nz_block[rank], MPI_INT, root, new_comm);
		MPI_Scatterv(NULL, 0, NULL,MPI_FLOAT, Hss_recv->val, Hss_nz_block[rank], MPI_FLOAT, root, new_comm);
		/*
		printf("\nIn rank %d, Hss blk : %d\n",rank,Hss_nz_block[rank] );
		printf("\nIn rank %d ,Row : %d\n", rank,recv_row[9]);
		printf("\nIn rank %d ,Col : %d\n", rank,recv_col[9]);
		printf("\nIn rank %d ,Val : %f\n", rank,recv_val[9]);
		*/

		Hss_recv->nnz = Hss_nz_block[rank];


		//finding the block inverses of the matrix
		compute_block_inverse(Hss_recv,rank);
	
	}

	//write a gatherv call here to gather the total no of rows,cols,vals and nonzeros in root rank


	//synchronize
	MPI_Barrier(MPI_COMM_WORLD);
	
	MPI_Finalize();

	return 0;
}