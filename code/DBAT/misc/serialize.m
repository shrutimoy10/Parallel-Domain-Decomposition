function [x,t]=serialize(s)
%SERIALIZE Serialize DBAT struct to vector.
%
%   X=SERIALIZE(S) extracts and reorders IO, EO, and OP parameters
%   from the DBAT struct S into the vector X. The parameter
%   selection and ordering is given by the S.serial field.
%
%   [X,T]=... also returns a same-sized cell array T with parameter
%   type strings.
%
%See also: DESERIALIZE, BUILDSERIALINDICES.

% Pre-allocate vector.
x=nan(s.bundle.serial.n,1);
% Insert element according to pre-calculated selection and ordering.
x(s.bundle.serial.IO.dest)=s.IO.val(s.bundle.serial.IO.src);
%fprintf("\nSERIAL: length of IO: %d\n",length(x(s.bundle.serial.IO.dest))); % roma - 0 stpierre - 8
x(s.bundle.serial.EO.dest)=s.EO.val(s.bundle.serial.EO.src);
%fprintf("\nSERIAL: length of EO: %d\n",length(x(s.bundle.serial.EO.dest))); % roma - 353 stpierre - 1434
x(s.bundle.serial.OP.dest)=s.OP.val(s.bundle.serial.OP.src);
%fprintf("\nSERIAL: length of OP: %d\n",length(x(s.bundle.serial.OP.dest))); % roma - 78963 stpierre - 53979

if nargout>1
    t=cell(size(x));
    t(s.bundle.serial.IO.dest)=s.IO.type(s.bundle.serial.IO.src);
    t(s.bundle.serial.EO.dest)=s.EO.type(s.bundle.serial.EO.src);
    t(s.bundle.serial.OP.dest)=s.OP.type(s.bundle.serial.OP.src);
end
