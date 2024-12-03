
// N - Bit Register File
// CE - Chip Enable, Active High
// RSTN - Asynchronous Reset, Active Low
module RegN ( D, Q, CE, RSTN, CLK) /* synthesis GSR="ENABLED" */;
parameter WIDTH = 8;
parameter INITIAL_VALUE = {WIDTH{1'b0}};

input wire[(WIDTH-1):0] D;
input wire CE, RSTN, CLK;
output reg[(WIDTH-1):0] Q = INITIAL_VALUE; // Output to Chip Registers 

always @(posedge CLK or negedge RSTN) begin
	if (RSTN == 1'b0) Q <= INITIAL_VALUE;
	else Q <= (CE == 1'b1) ? D : Q;
end
endmodule