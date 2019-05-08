//This function generates the block sizes into which the matrix
//must be divided when scattering to the different processors.
//since Hss blocks are 3x3 blocks
/* we have 78963 / 3 = 26321 which is not divisible by 3.
Hence, dividing the blocks into STRUCT_PARAMS/3 will not divide the non-zero
arrays accurately. So divide the blocks appropriately such that the smaller 3x3 blocks 
fit completely inside the matrix blocks.
*/
int* generate_block_sizes(int mat_size)
{
	int* block_sizes =  (int*) malloc(3*sizeof(int));
	int mat_blk_size;
	int diff,rem_blk;
	int size;

	mat_blk_size = mat_size/3;
	diff = mat_blk_size%3;

	if(diff == 0)
	{
		block_sizes[0] = block_sizes[1] = block_sizes[2] = mat_blk_size;
	}
	else
	{
		block_sizes[0] = mat_blk_size - diff;
		rem_blk = (mat_size - block_sizes[0])/2;

		block_sizes[1] = block_sizes[2] = rem_blk;
	}


	return block_sizes;
}



//this function converts the COO matrix into dense format
float** densify(coo_mat* mat,int blk_size,int rank)
{
	float** M;
	float* ptr;
	int i,j,k;
	unsigned int len;
	int blk_len;

	len = sizeof(float*) * blk_size + sizeof(float)* blk_size * blk_size;
	//printf("\nlen -> %u\n",len);
	M = (float**)malloc(len);


	ptr = M + blk_size; // ptr pointing to the first element of the 2D array, since malloc 
					//allocates contiguous memory.

	for(i = 0; i < blk_size ; i++)
		M[i] = ptr + blk_size * i; // each row pointer points to appropriate row now.

	//blk_len = mat->nnz/3;

	/*
	printf("\nBefore Assign....\n");
	printf("\nrow idx 0 : %d\n", mat->row_idx[0]);
	printf("\ncol idx 0 : %d\n", mat->col_idx[0]);
	printf("\nNNZ : %d\n", mat->nnz);
	*/
	for(i = 0 ; i< mat->nnz ; i++)
	{
		j = mat->row_idx[i]-mat->row_idx[0];
		k = mat->col_idx[i]-mat->col_idx[0];
		//printf("\nRank: %d ,j:%d, k:%d, val : %f\n",rank,j,k, mat->val[i]);
		M[j][k] = mat->val[i];
	}
	//printf("\nAfter Assign....\n");

	return M;

}


/*
This function computes the inverse of the matrix by computing the block inverse
of the 3x3 blocks using the formula Adj(A) / det(A).
After computing the inverse, the elements of each 3x3 block is stored in the
coo columns instead of dense storage.  
Since each block is 3x3, so the row,col, val arrays represent these blocks in groups of 9.
NOTE : The coo format is ordered by column.
Thus the each block is of the form:
		| a[0]  a[3]  a[6] |
	B = | a[1]  a[4]  a[7] |
		| a[2]  a[5]  a[8] |
		
*/
void compute_block_inverse(coo_mat* mat,int rank)
{
	int i=0;
	int j,k;
	float a[9];
	float adj[9];
	float det;
	float* orig_val = mat->val;
	float res;

	while(i < mat->nnz)
	{
		for(j=i,k=0;j < i+9,k<9;j++,k++)
		{
			a[k] = mat->val[j];
		}
		
		//computing the determinant of the block
		det = (a[0]*(a[4]*a[8] - a[5]*a[7])) - (a[3]*(a[1]*a[8] - a[2]*a[7])) + (a[6]*(a[1]*a[5] - a[2]*a[4]));
		//printf("\ndet : %f\n",det);
		//computing adjoint and dividing by det to get inverse of each element
		adj[0] = (a[4]*a[8] - a[5]*a[7])/det;  adj[3] = (a[5]*a[6] - a[3]*a[8])/det;  adj[6] = (a[3]*a[7] - a[4]*a[6])/det;
		adj[1] = (a[2]*a[7] - a[1]*a[8])/det;  adj[4] = (a[0]*a[8] - a[2]*a[6])/det;  adj[7] = (a[1]*a[6] - a[0]*a[7])/det;
		adj[2] = (a[1]*a[5] - a[2]*a[4])/det;  adj[5] = (a[2]*a[3] - a[0]*a[5])/det;  adj[8] = (a[0]*a[4] - a[1]*a[3])/det;

		//printf("\na[0] : %f\n",a[0]);
		//filling the inverse values into the val array
		for(j=i,k=0;j < i+9,k<9;j++,k++)
		{
			mat->val[j] = adj[k];
		}

		i = i + 9;
	}

}

//This function returns the no of non-zero indices in each row block of the given matrix.
//Counting only the row array is sufficient.
// Input param : prev_row_idx is the index num of mat->row_idx array at which the previous block ended.
//              so starting row_idx of next block will be mat->row_idx[prev_row_idx+1]
// Input param : row_idx_end is the row num at which the current block will end.
int nz_counts(coo_mat* mat,int rows_per_block,int prev_row_idx,int row_idx_end)
{
	int count = 0;
	int nz_row_idx_start = prev_row_idx+1;
	int i = nz_row_idx_start;

	printf("\nRow index end : %d\n", row_idx_end);

	while(mat->row_idx[i] <= row_idx_end)
	{
		count++;
		i++;
	}

	return count;
}



/* This function returns an array block_sizes which contains the sizes into which the arrays of indices of non-zero entries must be
split so as to scatter the row blocks of the matrices.
Input param size refers to the no of processors reserved for program execution. size will be the no of blocks
*/
int* generate_nz_block_size(coo_mat* mat,int size,int row)
{
	int*  block_sizes = (int*) malloc(size*sizeof(int));
	int  rows_per_block;
	int  m;
	int  i;
	int  prev_row_idx = -1;
	int  row_idx_end;

	m = row%size;
	rows_per_block = row / size; //no of rows in each block

	for(i=0;i<size;i++)
	{
		row_idx_end = i*rows_per_block + (rows_per_block-1);
		block_sizes[i] = nz_counts(mat,rows_per_block,prev_row_idx,row_idx_end);
		prev_row_idx +=  block_sizes[i]; //here the ith block ends in mat->row_idx
	}
	/*
	if(m ==0)
	{
		row_idx_end = (size-1)*rows_per_block + (rows_per_block-1);
		block_sizes[size-1] = nz_counts(mat,rows_per_block,prev_row_idx,row_idx_end);
	}*/
	if(m != 0)
	{
		//row_idx_end = (size-1)*rows_per_block + (rows_per_block+m-1);
		//block_sizes[size-1] = nz_counts(mat,rows_per_block,prev_row_idx,row_idx_end);
		block_sizes[size-1] += mat->nnz - prev_row_idx-1; // -1 because C is 0 indexed and nnz is 1 indexed
	}
	

	
	/*****TEST****/
	/*
	int sum = 0;
	for(i=0;i<size;i++)
	{
		printf("\nBlock %d : %d\n",i,block_sizes[i] );
		sum += block_sizes[i];
	}

	int nz_diff = sum - mat->nnz;
	printf("\nSum nz : %d\n", sum);
	printf("\nmat->nnz : %d\n", mat->nnz);
	printf("\nThe error is : %d\n",nz_diff );
	*/
	return block_sizes;
}

//This function splits the rhs b into b1 and b2 with b1 being a CAM_PARAMSx1 vector  and
//b2 being a STRUCT_PARAMSx1 vector
float* split_b(float* b,int vec_size,int offset)
{
	int i;
	float* bi = (float*) malloc (vec_size * sizeof(float));

	for(i = 0;i < vec_size;i++)
		bi[i] = b[offset+i];

	return bi;
}

//This function returns an array containing the block sizes for scattering the b vectors
int* generate_b_block_size(int rows,int size)
{
	int* b_row_block = (int*) malloc (size * sizeof(int));
	int  m;
	int  rows_per_block = rows/size; 
	int  i;

	m = rows%size;

	for(i=0;i<size;i++)
		b_row_block[i] = rows_per_block;

	if(m != 0)
		b_row_block[size-1] += m;


	return b_row_block;
}