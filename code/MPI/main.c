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

	MPI_Init(0,0);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	coo_mat* Hcc;
	coo_mat* Hcs;
	coo_mat* Hsc;
	coo_mat* Hss;
	coo_mat* Hss_inv;
	float*   b;
	int      Hss_nz_block[3];
	int* 	 Hss_mat_block;
	int 	 displs[3];
	int*     row;
	int*     col;
	float*   val;
	int* 	 Hcc_row_block_size;
	int* 	 Hcs_row_block_size;
	int* 	 Hsc_row_block_size;
	int* 	 Hss_row_block_size;
	float*	 b1;
	float*	 b2;
	int*	 b1_block_size;
	int* 	 b2_block_size;
	int 	 row_block_displs_Hcc[size]; 
	int 	 row_block_displs_Hcs[size];
	int 	 row_block_displs_Hsc[size];
	int 	 row_block_displs_Hss[size];
	int 	 row_block_displs_b1[size];
	int 	 row_block_displs_b2[size];
	float* 	 b1_Recv;
	float* 	 b2_Recv;
	int 	 i;

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
	Hss_mat_block = generate_block_sizes(STRUCT_PARAMS); 
	
	coo_mat* Hss_recv = (coo_mat*) malloc(sizeof(coo_mat));

	Hcc = read_coo_matrix("Hcc");
	Hcs = read_coo_matrix("Hcs");
	Hsc = read_coo_matrix("Hsc");
	Hss = read_coo_matrix("Hss");

	// read the RHS of the system
	b = read_b(CAM_PARAMS+STRUCT_PARAMS); 

	//read the matrices in the master process.
	if(rank == root)
	{
			

		//printf("\nIn Hss, nnz = %d\n",Hss->nnz); // 236889 = 3 x 78963

		Hss_nz_block[0] = Hss_mat_block[0]*3;
		Hss_nz_block[1] = Hss_mat_block[1]*3;
		Hss_nz_block[2] = Hss_mat_block[2]*3;

		//printf("\nBlock 1: %d Block 2 : %d block 3: %d\n", Hss_mat_block[0], Hss_mat_block[1],Hss_mat_block[2]);
		//printf("\nNon Zeros -> Block 1: %d Block 2 : %d block 3: %d\n", Hss_nz_block[0], Hss_nz_block[1],Hss_nz_block[2]);

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
		
		Hss_recv->nnz = Hss_nz_block[rank];


		//finding the block inverses of the matrix
		compute_block_inverse(Hss_recv,rank);


		//gather the inverted value into the original Hss structure to replace the old values
		MPI_Gatherv(Hss_recv->row_idx, Hss_recv->nnz, MPI_FLOAT, Hss->row_idx,Hss_nz_block,displs,MPI_INT,root,new_comm);
		MPI_Gatherv(Hss_recv->col_idx, Hss_recv->nnz, MPI_FLOAT, Hss->col_idx,Hss_nz_block,displs,MPI_INT,root,new_comm);
		MPI_Gatherv(Hss_recv->val, Hss_recv->nnz, MPI_FLOAT, Hss->val,Hss_nz_block,displs,MPI_FLOAT,root,new_comm);
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
		
		Hss_recv->nnz = Hss_nz_block[rank];


		//finding the block inverses of the matrix
		compute_block_inverse(Hss_recv,rank);

		
		//for gathering the values from different matrices
		row = (int*) malloc (Hss->nnz * sizeof(int));
		col = (int*) malloc (Hss->nnz * sizeof(int));
		val = (float*) malloc (Hss->nnz * sizeof(float));

		//gather the inverted value into the original Hss structure to replace the old values
		MPI_Gatherv(Hss_recv->row_idx, Hss_recv->nnz, MPI_FLOAT, row,Hss_nz_block,displs,MPI_INT,root,new_comm);
		MPI_Gatherv(Hss_recv->col_idx, Hss_recv->nnz, MPI_FLOAT, col,Hss_nz_block,displs,MPI_INT,root,new_comm);
		MPI_Gatherv(Hss_recv->val, Hss_recv->nnz, MPI_FLOAT, val,Hss_nz_block,displs,MPI_FLOAT,root,new_comm);

		free(row);
		free(col);
		free(val);
	}

	if(rank == root || rank == 1 || rank == 2)
		MPI_Barrier(new_comm);

	if(rank == root)
		MPI_Comm_free(&new_comm);

	free(Hss_recv);

	//synchronize
	MPI_Barrier(MPI_COMM_WORLD);

	coo_mat* Hcc_Recv = (coo_mat*) malloc (sizeof(coo_mat));
	coo_mat* Hcs_Recv = (coo_mat*) malloc (sizeof(coo_mat));
	coo_mat* Hsc_Recv = (coo_mat*) malloc (sizeof(coo_mat));
	coo_mat* Hss_Recv = (coo_mat*) malloc (sizeof(coo_mat));

	//scatter the submatrices for LU solve
	if(rank == root)
	{
		Hcc_row_block_size = generate_nz_block_size(Hcc,size,CAM_PARAMS);
		Hcs_row_block_size = generate_nz_block_size(Hsc,size,CAM_PARAMS); //interchanging use of Hcs and Hsc as they are symmetric
		Hsc_row_block_size = generate_nz_block_size(Hcs,size,STRUCT_PARAMS); 
		Hss_row_block_size = generate_nz_block_size(Hss,size,STRUCT_PARAMS);
		//printf("\n1\n");
		//Since the rhs b is also split into b1 & b2 such that b1 is a CAM_PARAMS x 1 vector and
		// b2 is a STRUCT_PARAMS x 1 vector, each of b1 and b2 will also be split in the same way
		// as the row blocks of the submatrices
		b1 = split_b(b,CAM_PARAMS,0);
		b2 = split_b(b,STRUCT_PARAMS,CAM_PARAMS);
		b1_block_size = generate_b_block_size(CAM_PARAMS,size);
		b2_block_size = generate_b_block_size(STRUCT_PARAMS,size);
		//printf("\n2\n");
		//broadcasting the non zero block sizes
		MPI_Bcast(Hcc_row_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(Hcs_row_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(Hsc_row_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(Hss_row_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(b1_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(b2_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		//printf("\n3\n");
		//setting the displacement values for scatterv
		//count of previous block should be added to get the correct displacement
		row_block_displs_Hcc[0] = 0;
		row_block_displs_Hcs[0] = 0;
		row_block_displs_Hsc[0] = 0;
		row_block_displs_Hss[0] = 0;
		row_block_displs_b1[0] = 0;
		row_block_displs_b2[0] = 0;
		for(i = 1;i < size;i++)
		{
			row_block_displs_Hcc[i] = row_block_displs_Hcc[i-1] + Hcc_row_block_size[i-1];
			row_block_displs_Hcs[i] = row_block_displs_Hcs[i-1] + Hcs_row_block_size[i-1];
			row_block_displs_Hsc[i] = row_block_displs_Hsc[i-1] + Hsc_row_block_size[i-1];
			row_block_displs_Hss[i] = row_block_displs_Hss[i] + Hss_row_block_size[i-1];
			row_block_displs_b1[i] = row_block_displs_b1[i-1] + b1_block_size[i-1];
			row_block_displs_b2[i] = row_block_displs_b2[i-1] + b2_block_size[i-1];
		}
		//printf("\n4\n");
		//allocating memory for individual matrices
		Hcc_Recv->row_idx = (int*)malloc(Hcc_row_block_size[rank]*sizeof(int));
		Hcc_Recv->col_idx = (int*)malloc(Hcc_row_block_size[rank]*sizeof(int));
		Hcc_Recv->val = (float*)malloc(Hcc_row_block_size[rank]*sizeof(float));

		Hcs_Recv->row_idx = (int*)malloc(Hcs_row_block_size[rank]*sizeof(int));
		Hcs_Recv->col_idx = (int*)malloc(Hcs_row_block_size[rank]*sizeof(int));
		Hcs_Recv->val = (float*)malloc(Hcs_row_block_size[rank]*sizeof(float));

		Hsc_Recv->row_idx = (int*)malloc(Hsc_row_block_size[rank]*sizeof(int));
		Hsc_Recv->col_idx = (int*)malloc(Hsc_row_block_size[rank]*sizeof(int));
		Hsc_Recv->val = (float*)malloc(Hsc_row_block_size[rank]*sizeof(float));

		Hss_Recv->row_idx = (int*)malloc(Hss_row_block_size[rank]*sizeof(int));
		Hss_Recv->col_idx = (int*)malloc(Hss_row_block_size[rank]*sizeof(int));
		Hss_Recv->val = (float*)malloc(Hss_row_block_size[rank]*sizeof(float));

		b1_Recv = (float*) malloc (b1_block_size[rank]*sizeof(float));
		b2_Recv = (float*) malloc (b2_block_size[rank]*sizeof(float));
		//printf("\n5\n");

		//scattering the matrices
		MPI_Scatterv(Hcc->row_idx,Hcc_row_block_size,row_block_displs_Hcc,MPI_INT,Hcc_Recv->row_idx,Hcc_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(Hcc->col_idx,Hcc_row_block_size,row_block_displs_Hcc,MPI_INT,Hcc_Recv->col_idx,Hcc_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(Hcc->val,Hcc_row_block_size,row_block_displs_Hcc,MPI_FLOAT,Hcc_Recv->val,Hcc_row_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);
		
		MPI_Scatterv(Hcs->row_idx,Hcs_row_block_size,row_block_displs_Hcs,MPI_INT,Hcs_Recv->row_idx,Hcs_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(Hcs->col_idx,Hcs_row_block_size,row_block_displs_Hcs,MPI_INT,Hcs_Recv->col_idx,Hcs_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(Hcs->val,Hcs_row_block_size,row_block_displs_Hcs,MPI_FLOAT,Hcs_Recv->val,Hcs_row_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);
		
		MPI_Scatterv(Hsc->row_idx,Hsc_row_block_size,row_block_displs_Hsc,MPI_INT,Hsc_Recv->row_idx,Hsc_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(Hsc->col_idx,Hsc_row_block_size,row_block_displs_Hsc,MPI_INT,Hsc_Recv->col_idx,Hsc_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(Hsc->val,Hsc_row_block_size,row_block_displs_Hsc,MPI_FLOAT,Hsc_Recv->val,Hsc_row_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);

		MPI_Scatterv(Hss->row_idx,Hss_row_block_size,row_block_displs_Hss,MPI_INT,Hss_Recv->row_idx,Hss_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(Hss->col_idx,Hss_row_block_size,row_block_displs_Hss,MPI_INT,Hss_Recv->col_idx,Hss_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(Hss->val,Hss_row_block_size,row_block_displs_Hss,MPI_FLOAT,Hss_Recv->val,Hss_row_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);
		
		MPI_Scatterv(b1,b1_block_size,row_block_displs_b1,MPI_FLOAT,b1_Recv,b1_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);
		MPI_Scatterv(b2,b2_block_size,row_block_displs_b2,MPI_FLOAT,b2_Recv,b2_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);
	
		//printf("\n6\n");
		
		printf("\nRank %d ,Disp : %d, Hsc[0][0] = %15.5f\n", rank,row_block_displs_Hsc[rank],Hsc_Recv->val[0]);
	}
	else
	{
		//allocating memory for receiving buffer
		Hcc_row_block_size = (int*) malloc (size * sizeof(int));
		Hcs_row_block_size = (int*) malloc (size * sizeof(int));
		Hsc_row_block_size = (int*) malloc (size * sizeof(int));
		Hss_row_block_size = (int*) malloc (size * sizeof(int));
		b1_block_size = (int*) malloc (size * sizeof(int));
		b2_block_size = (int*) malloc (size * sizeof(int));

		//receiving the non zero block sizes
		MPI_Bcast(Hcc_row_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(Hcs_row_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(Hsc_row_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(Hss_row_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(b1_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		MPI_Bcast(b2_block_size,size,MPI_INT,root,MPI_COMM_WORLD);
		//printf("\nRank : %d, 1\n", rank);

		//allocating memory for individual matrices
		Hcc_Recv->row_idx = (int*)malloc(Hcc_row_block_size[rank]*sizeof(int));
		Hcc_Recv->col_idx = (int*)malloc(Hcc_row_block_size[rank]*sizeof(int));
		Hcc_Recv->val = (float*)malloc(Hcc_row_block_size[rank]*sizeof(float));

		Hcs_Recv->row_idx = (int*)malloc(Hcs_row_block_size[rank]*sizeof(int));
		Hcs_Recv->col_idx = (int*)malloc(Hcs_row_block_size[rank]*sizeof(int));
		Hcs_Recv->val = (float*)malloc(Hcs_row_block_size[rank]*sizeof(float));

		Hsc_Recv->row_idx = (int*)malloc(Hsc_row_block_size[rank]*sizeof(int));
		Hsc_Recv->col_idx = (int*)malloc(Hsc_row_block_size[rank]*sizeof(int));
		Hsc_Recv->val = (float*)malloc(Hsc_row_block_size[rank]*sizeof(float));

		Hss_Recv->row_idx = (int*)malloc(Hss_row_block_size[rank]*sizeof(int));
		Hss_Recv->col_idx = (int*)malloc(Hss_row_block_size[rank]*sizeof(int));
		Hss_Recv->val = (float*)malloc(Hss_row_block_size[rank]*sizeof(float));

		b1_Recv = (float*) malloc (b1_block_size[rank]*sizeof(float));
		b2_Recv = (float*) malloc (b2_block_size[rank]*sizeof(float));
		//printf("\nRank : %d, 2\n", rank);

		//setting the displacement values for scatterv
		//count of previous block should be added to get the correct displacement
		row_block_displs_Hcc[0] = 0;
		row_block_displs_Hcs[0] = 0;
		row_block_displs_Hsc[0] = 0;
		row_block_displs_Hss[0] = 0;
		row_block_displs_b1[0] = 0;
		row_block_displs_b2[0] = 0;
		for(i = 1;i < size;i++)
		{
			row_block_displs_Hcc[i] = row_block_displs_Hcc[i-1] + Hcc_row_block_size[i-1];
			row_block_displs_Hcs[i] = row_block_displs_Hcs[i-1] + Hcs_row_block_size[i-1];
			row_block_displs_Hsc[i] = row_block_displs_Hsc[i-1] + Hsc_row_block_size[i-1];
			row_block_displs_Hss[i] = row_block_displs_Hss[i] + Hss_row_block_size[i-1];
			row_block_displs_b1[i] = row_block_displs_b1[i-1] + b1_block_size[i-1];
			row_block_displs_b2[i] = row_block_displs_b2[i-1] + b2_block_size[i-1];
		}

		MPI_Scatterv(NULL,0,NULL,MPI_INT,Hcc_Recv->row_idx,Hcc_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(NULL,0,NULL,MPI_INT,Hcc_Recv->col_idx,Hcc_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(NULL,0,NULL,MPI_FLOAT,Hcc_Recv->val,Hcc_row_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);
		
		MPI_Scatterv(NULL,0,NULL,MPI_INT,Hcs_Recv->row_idx,Hcs_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(NULL,0,NULL,MPI_INT,Hcs_Recv->col_idx,Hcs_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(NULL,0,NULL,MPI_FLOAT,Hcs_Recv->val,Hcs_row_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);

		MPI_Scatterv(NULL,0,NULL,MPI_INT,Hsc_Recv->row_idx,Hsc_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(NULL,0,NULL,MPI_INT,Hsc_Recv->col_idx,Hsc_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(NULL,0,NULL,MPI_FLOAT,Hsc_Recv->val,Hsc_row_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);
		
		MPI_Scatterv(NULL,0,NULL,MPI_INT,Hss_Recv->row_idx,Hss_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(NULL,0,NULL,MPI_INT,Hss_Recv->col_idx,Hss_row_block_size[rank],MPI_INT,root,MPI_COMM_WORLD);
		MPI_Scatterv(NULL,0,NULL,MPI_FLOAT,Hss_Recv->val,Hss_row_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);

		MPI_Scatterv(NULL,0,NULL,MPI_FLOAT,b1_Recv,b1_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);
		MPI_Scatterv(NULL,0,NULL,MPI_FLOAT,b2_Recv,b2_block_size[rank],MPI_FLOAT,root,MPI_COMM_WORLD);
		//printf("\nRank : %d, 3\n", rank);
		printf("\nRank %d , Disp : %d, Hsc[0][0] = %15.5f\n", rank,row_block_displs_Hsc[rank],Hsc_Recv->val[0]);
	}
	

	MPI_Finalize();

	return 0;
}