`timescale 1ns/1ns

module Mux2To1( IN, OUT, SEL);
input wire[1:0] IN;
input wire  SEL;
output reg OUT;

	
always @(*) 
begin
	case (SEL)
		1'b0 : OUT <= IN[0];
		1'b1 : OUT <= IN[1];
	endcase	
end	
endmodule