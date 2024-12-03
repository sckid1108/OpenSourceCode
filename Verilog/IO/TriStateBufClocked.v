module TriStateBufClocked(I,O,EN,CE,RST,CLK) /* synthesis GSR="ENABLED" */;
input I,EN,CE,RST,CLK;
output O;
reg Q;

assign O = (EN == 1'b1) ? Q : 1'bZ;

always @(posedge CLK or posedge RST)
begin
	if (RST == 1'b1) 
		Q <= 1'b0;
	else 
		Q <= ( CE == 1'b1 ) ? I : Q;
end

endmodule
