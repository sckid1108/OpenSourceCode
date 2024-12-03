
module OneShot(OUT, RSTN, CLK) /* synthesis GSR="ENABLED" */;
output wire OUT;
input wire RSTN, CLK;

reg [1:0] qd;

assign OUT = qd[1];
	
always @(posedge CLK or negedge RSTN) begin
	if (RSTN == 1'b0) qd <= 2'b01;
	else qd <= {qd[0],1'b0};
end

endmodule