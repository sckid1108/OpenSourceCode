module BiDirBufClocked(A,B,EN,CE,RST,CLK) /* synthesis GSR="ENABLED" */;
inout A,B;
input EN,CE,RST,CLK;

reg Q0,Q1;

assign B = (EN == 1'b1) ? Q0 : 1'bZ;
assign A = (EN == 1'b0) ? Q1 : 1'bZ;

always @(posedge CLK or posedge RST)
begin
	if (RST == 1'b1) begin
		Q0 <= 1'b0;
		Q1 <= 1'b0;
	end
	else if (CE == 1'b1) begin
		Q0 <= A;
		Q1 <= B;
	end 
	else begin
		Q0 <= Q0;
		Q1 <= Q1;
	end
end

endmodule
