
module BiDirBuf (A,B,EN);
inout A,B;
input EN;

assign B = (EN == 1'b1) ? A : 1'bZ;
assign A = (EN == 1'b0) ? B : 1'bZ;

endmodule
