// N - Bit Parallel To Serial Shift Register
// LOAD - Load Input, Active High
// CE - Chip Enable, Active High
// RSTN - Asynchronous Reset, Active Low
module ParallelToSerial ( D, Q, LOAD, CE, RSTN, CLK);
parameter WIDTH = 8;
parameter INITIAL_VALUE = { WIDTH{1'b0} };

input wire[(WIDTH-1):0] D;
input wire LOAD, CE, RSTN, CLK;
output wire Q; // Serial Shift Output

reg [(WIDTH-1):0] tmp;

assign Q = tmp[(WIDTH-1)];

	always @(posedge CLK or negedge RSTN) begin
		if (RSTN == 1'b0) 
			tmp <= INITIAL_VALUE;
		else begin
			if ( CE == 1'b1 )
              if ( LOAD == 1'b1 )
                  tmp <= D;
              else               
				tmp <= { tmp[(WIDTH-2):0], 1'b0 };
			else
				tmp <= tmp;
		end
	end

endmodule