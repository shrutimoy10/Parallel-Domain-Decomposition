//this file contains code for reading the coo matrix from the respective files.
//input param block type refers to the type of block to be read



//this function converts the COO matrix into dense format
double** densify(int row,int col,int* row_idx,int* col_idx,double* val,int nnz)
{
	double** M;
	double* ptr;
	int i,j,k;
	unsigned int len;

	len = sizeof(double*)*row + sizeof(double)*row*col;
	printf("\nlen -> %u\n",len);
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


//This function reads the COO representation from the file and stores the resp indices
//in the arrays row_idx[],col_idx[],val[]. The function then returns the dense matrix.
double** read_file(char* input_filename)
{
	double** M;
	int row_max;
    int row_min;
    int col_max;
    int col_min;
    int row;
    int col;
    int nnz;
    int *row_idx;
    int *col_idx;
    double *val;

	r8st_header_read(input_filename, &row_min, &row_max, &col_min, &col_max, &row, &col, &nnz );

	row_idx = (int*)malloc(nnz*sizeof(int));
	col_idx = (int*)malloc(nnz*sizeof(int));
	val     = (double*)malloc(nnz*sizeof(double));

	r8st_data_read (input_filename, row, col, nnz, row_idx, col_idx, val);

	M = densify(row, col, row_idx, col_idx, val, nnz); //returns a dense matrix

	free(row_idx);
	free(col_idx);
	free(val);

	return M;	
}

double** read_coo_matrix(char* block_type,int row,int col)
{
	double** H;


	if (strcmp(block_type,"Hcc") == 0)
	{
		printf("\nHcc\n");
		H = read_file("../Results/Matrices/Hcc.txt");
	}

	else if (strcmp(block_type,"Hcs") == 0)
	{
		printf("\nHcs\n");
		H = read_file("../Results/Matrices/Hcs.txt");
	}

	else if (strcmp(block_type,"Hsc") == 0)
	{
		printf("\nHsc\n");
		H = read_file("../Results/Matrices/Hsc.txt");
	}

	else if (strcmp(block_type,"Hss") == 0)
	{
		printf("\nHss\n");
		H = read_file("../Results/Matrices/Hss.txt");
	}

	return H;
}

