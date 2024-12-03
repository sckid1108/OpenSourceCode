`timescale 1ns/1ns

module Mux4To1( IN, OUT, SEL);
input wire[3:0] IN;
input wire[1:0] SEL;
output reg OUT;

always @(*) 
begin
	case (SEL)
		2'b00 : OUT <= IN[0];
		2'b01 : OUT <= IN[1];
		2'b10 : OUT <= IN[2];
		2'b11 : OUT <= IN[3];
	endcase	
end	
endmodule