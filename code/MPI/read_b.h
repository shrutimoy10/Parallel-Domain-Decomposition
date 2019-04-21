		
double* read_b(int row)
{

		double* b;
		int k;
		FILE* fptr;

		if((fptr = fopen("../../../Matrices/b.txt","r")) == NULL)
		{
			printf("Error in opening b.txt");
			exit(1);
		}

		b = (double*) malloc(row*sizeof(double));
		
		for(k=0;k<row;k++)
			fscanf(fptr,"%lf[^\n]",(b+k));
		
		fclose(fptr);

		return b;
}

		