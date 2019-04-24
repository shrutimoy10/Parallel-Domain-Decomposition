		
float* read_b(int row)
{

		float* b;
		int k;
		FILE* fptr;

		if((fptr = fopen("../../../Matrices/b.txt","r")) == NULL)
		{
			printf("Error in opening b.txt");
			exit(1);
		}

		b = (float*) malloc(row*sizeof(float));
		
		for(k=0;k<row;k++)
			fscanf(fptr,"%f[^\n]",(b+k));
		
		fclose(fptr);

		return b;
}

		