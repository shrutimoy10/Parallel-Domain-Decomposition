

//structure for containing the row_idx, col_idx and val arrays
//of the COO representation of the matrix.
typedef struct 
{
	int* row_idx;
	int* col_idx;
	float* val;
	int nnz;
}coo_mat;



//This function reads the COO representation from the file and stores the resp indices
//in the arrays row_idx[],col_idx[],val[]. The function then returns a pointer to the structure containing the coo
//representation.
coo_mat* read_file(char* input_filename)
{
	//ouble** M;
	int row_max;
    int row_min;
    int col_max;
    int col_min;
    int row;
    int col;
    int nnz;
    //int *row_idx;
    //int *col_idx;
    //double *val;
    coo_mat* mat_read = (coo_mat*)malloc(sizeof(coo_mat));


	r8st_header_read(input_filename, &row_min, &row_max, &col_min, &col_max, &row, &col, &nnz );

	mat_read->row_idx = (int*)malloc(nnz*sizeof(int));
	mat_read->col_idx = (int*)malloc(nnz*sizeof(int));
	mat_read->val     = (float*)malloc(nnz*sizeof(float));
	mat_read->nnz     = nnz;

	r8st_data_read (input_filename, row, col, nnz, mat_read->row_idx, mat_read->col_idx, mat_read->val);

	//M = densify(row, col, row_idx, col_idx, val, nnz); //returns a dense matrix

	//free(row_idx);
	//free(col_idx);
	//free(val);

	return mat_read;	
}


//this file contains code for reading the coo matrix from the respective files.
//input param block type refers to the type of block to be read
coo_mat* read_coo_matrix(char* block_type)
{
	coo_mat* mat_read;


	if (strcmp(block_type,"Hcc") == 0)
	{
		mat_read = read_file("../../../Matrices/Hcc.txt");
	}

	else if (strcmp(block_type,"Hcs") == 0)
	{
		mat_read = read_file("../../../Matrices/Hcs.txt");
	}

	else if (strcmp(block_type,"Hsc") == 0)
	{
		mat_read = read_file("../../../Matrices/Hsc.txt");
	}

	else if (strcmp(block_type,"Hss") == 0)
	{
		mat_read = read_file("../../../Matrices/Hss.txt");
	}

	return mat_read;
}

