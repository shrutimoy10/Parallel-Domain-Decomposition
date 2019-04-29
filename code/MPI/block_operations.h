//This function generates the block sizes into which the matrix
//must be divided when scattering to the different processors
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

//this is the function to compute the block inverses