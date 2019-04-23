//This code computes the inverse of the input block diagonal matrix.
//Since the individual blocks are 3x3 so their inverse can be computed 
//as (adjoint a)/(det a). So the complete Hss matrix has to be divided into
//blocks of 3 to utilize the structure of the 3x3 blocks.
