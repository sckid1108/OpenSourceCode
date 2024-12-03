`timescale 1ns/1ns  // time unit / time precision

// N-Bit Input Shift register
// Active Low Reset
// Active High Clock Enable
module ShiftReg(D, Q, RSTN, CE, CLK);
parameter N = 8;
parameter INIT = { N{1'b0} };

input logic D,RSTN,CE,CLK;
output logic [N-1:0] Q;

always_ff @(posedge CLK or negedge RSTN) begin
	if (RSTN == 1'b1)
		Q <= (CE == 1'b1) ? {Q[N-2:0], D} : Q;
	else
		Q <= INIT;
end

endmodule