
module BiDirBuf2 (BIDIR,I,O,EN);
inout BIDIR;
input I,EN;
output O;

assign BIDIR = (EN == 1'b1) ? I : 1'bZ;
assign O = (EN == 1'b0) ? BIDIR : 1'bZ;

endmodule
