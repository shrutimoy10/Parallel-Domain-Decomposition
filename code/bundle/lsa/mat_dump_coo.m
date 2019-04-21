%this function writes the input matrix into 
%the Hs.txt file

function mat_dump_coo(H,mat)
    fprintf('\n=======Dumping matrices=======\n');
    if strcmp(mat,'Hcc')
       %non_zeros = nnz(H);
       [i,j,val] = find(H);     % find to get index & value vectors
       data_dump = [i-1,j-1,val];     % save this in data_dump % making the entries 0 indexed
       %disp(data_dump(1:5,:)');
       fid = fopen('/home/iiit/shrutimoy.das/BundleAdjustment/Hcc.txt','w');    % open file in write mode
       %fprintf(fid,'%d\n',non_zeros);
       fprintf( fid,'\t%d\t%d\t%g\n',data_dump');     % print the JTJ to the file in COO
       fclose(fid);
    elseif strcmp(mat,'Hcs')
       %non_zeros = nnz(H);
       [i,j,val] = find(H);     % find to get index & value vectors
       data_dump = [i-1,j-1,val];     % save this in data_dump
       fid = fopen('/home/iiit/shrutimoy.das/BundleAdjustment/Hcs.txt','w');    % open file in write mode
       %fprintf(fid,'%d\n',non_zeros);
       fprintf( fid,'\t%d\t%d\t%g\n',data_dump');     % print the JTJ to the file in COO
       fclose(fid);
    elseif strcmp(mat,'Hsc')
       %non_zeros = nnz(H);
       [i,j,val] = find(H);     % find to get index & value vectors
       data_dump = [i-1,j-1,val];     % save this in data_dump
       fid = fopen('/home/iiit/shrutimoy.das/BundleAdjustment/Hsc.txt','w');    % open file in write mode
       %fprintf(fid,'%d\n',non_zeros);
       fprintf( fid,'\t%d\t%d\t%g\n',data_dump');     % print the JTJ to the file in COO
       fclose(fid);        
    elseif strcmp(mat,'Hss')
       %non_zeros = nnz(H);
       [i,j,val] = find(H);     % find to get index & value vectors
       data_dump = [i-1,j-1,val];     % save this in data_dump
       fid = fopen('/home/iiit/shrutimoy.das/BundleAdjustment/Hss.txt','w');    % open file in write mode
       %fprintf(fid,'%d\n',non_zeros);
       fprintf( fid,'\t%d\t%d\t%g\n',data_dump');     % print the JTJ to the file in COO
       fclose(fid);   
    elseif strcmp(mat,'b')       % save this in data_dump
       %non_zeros = nnz(H); 
       fid = fopen('/home/iiit/shrutimoy.das/BundleAdjustment/b.txt','w');    % open file in write mode
       %fprintf(fid,'%d\n',non_zeros);
       fprintf( fid,'%g\n',H);     % print the JTJ to the file in COO
       fclose(fid);  
    end
	
end 