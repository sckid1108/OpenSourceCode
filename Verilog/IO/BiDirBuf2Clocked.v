module BiDirBuf2Clocked(BIDIR,I,O,EN,CE,RST,CLK) /* synthesis GSR="ENABLED" */; 
inout BIDIR;
output O;
input I,EN,CE,RST,CLK;

reg Q0,Q1;

assign BIDIR = (EN == 1'b1) ? Q0 : 1'bZ;
assign O = Q1;

always @(posedge CLK or posedge RST)
begin
	if (RST == 1'b1) begin
		Q0 <= 1'b0;
		Q1 <= 1'b0;
	end
	else if (CE == 1'b1) begin
		Q0 <= I;
		Q1 <= BIDIR;
	end
	else begin
		Q0 <= Q0;
		Q1 <= Q1;
	end
end

endmodule
