//this file contains code for reading the coo matrix from the respective files.
//input param block type refers to the type of block to be read

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
	r8st_data_read (input_filename, row, col, nnz, row_idx, col_idx, val);

	//densify(*M, row, col, row_idx, col_idx, val, nnz); //returns a dense matrix

	return M;	
}

double** read_coo_matrix(char* block_type,int row,int col)
{
	double** H;


	if (strcmp(block_type,"Hcc") == 0)
	{
		H = read_file("/home/iiit/shrutimoy.das/Parallel-Domain-Decomposition/Results/Matrices/Hcc.txt");
	}

	else if (strcmp(block_type,"Hcs") == 0)
	{
		H = read_file("/home/iiit/shrutimoy.das/Parallel-Domain-Decomposition/Results/Matrices/Hcs.txt");
	}

	else if (strcmp(block_type,"Hsc") == 0)
	{
		H = read_file("/home/iiit/shrutimoy.das/Parallel-Domain-Decomposition/Results/Matrices/Hsc.txt");
	}

	else if (strcmp(block_type,"Hss") == 0)
	{
		H = read_file("/home/iiit/shrutimoy.das/Parallel-Domain-Decomposition/Results/Matrices/Hss.txt");
	}
	/*
	else if (strcmp(block_type,"b") == 0)
	{
		if((fptr = fopen("/home/iiit/shrutimoy.das/Parallel-Domain-Decomposition/Results/Matrices/b.txt","r")) == NULL)
		{
			printf("Error in opening b.txt");
			exit(1);
		}
		fscanf(fptr,"%d[^\n]",&nnz);
		//printf("NNZ = %d\n", nnz); // 79316
		H = read_file(fptr,row,col,nnz);
		fclose(fptr);
	} 
	*/
	return H;
}

