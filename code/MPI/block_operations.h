//this function converts the COO matrix into dense format
double** densify(int row,int col,int* row_idx,int* col_idx,double* val,int nnz)
{
	double** M;
	double* ptr;
	int i,j,k;
	unsigned int len;

	len = sizeof(double*)*row + sizeof(double)*row*col;
	//printf("\nlen -> %u\n",len);
	M = (double**)malloc(len);

	if(M == NULL)
		printf("\nError in pointer allocation.\n");

	ptr = M + row; // ptr pointing to the first element of the 2D array, since malloc 
					//allocates contiguous memory.

	for(i=0;i<row;i++)
		M[i] = ptr + col*i; // each row pointer points to appropriate row now.

	for(i=0;i<nnz;i++)
	{
		j = row_idx[i];
		k = col_idx[i];
		M[j][k] = val[i];
	}

	printf("\nM -> %lf\n", M[0][2]); // -1.4099e+07

	return M;

}

//this is the function to compute the block inverses