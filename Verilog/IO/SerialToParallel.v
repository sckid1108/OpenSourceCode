
// N - Bit Serial To Parallel Shift Register
// CE - Chip Enable, Active High
// RSTN - Asynchronous Reset, Active Low
module SerialToParallel ( D, Q, CE, RSTN, CLK) /* synthesis GSR="ENABLED" */;
parameter WIDTH = 8;
parameter INITIAL_VALUE = { WIDTH{1'b0} };

input wire D; // Serial Shift Input
input wire CE, RSTN, CLK;
output wire [(WIDTH-1):0] Q; 

reg [(WIDTH-1):0] tmp;

	assign Q = tmp;

	always @(posedge CLK or negedge RSTN) begin
		if (RSTN == 1'b0) 
			tmp <= INITIAL_VALUE;
		else begin
			if ( CE == 1'b1 )
				tmp <= { tmp[(WIDTH-2):0], D };
			else
				tmp <= tmp;
		end
	end
endmodule