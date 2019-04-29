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

	printf("\nBefore Assign....\n");
	printf("\nrow idx 0 : %d\n", mat->row_idx[0]);
	printf("\ncol idx 0 : %d\n", mat->col_idx[0]);
	printf("\nNNZ : %d\n", mat->nnz);
	for(i = 0 ; i< mat->nnz ; i++)
	{
		j = mat->row_idx[i]-mat->row_idx[0];
		k = mat->col_idx[i]-mat->col_idx[0];
		//printf("\nRank: %d ,j:%d, k:%d, val : %f\n",rank,j,k, mat->val[i]);
		M[j][k] = mat->val[i];
	}
	printf("\nAfter Assign....\n");

	return M;

}

//this is the function to compute the block inverses