

module TriStateBuf(I,O,EN);
input I,EN;
output O;

assign O = (EN == 1'b1) ? I : 1'bZ;

endmodule
