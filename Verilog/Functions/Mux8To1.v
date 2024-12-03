`timescale 1ns/1ns

module Mux8To1( IN, OUT, SEL);
input wire[7:0] IN;
input wire[2:0] SEL;
output reg OUT;

	
always @(*) 
begin
	case (SEL)
		3'b000 : OUT <= IN[0];
		3'b001 : OUT <= IN[1];
		3'b010 : OUT <= IN[2];
		3'b011 : OUT <= IN[3];
		3'b100 : OUT <= IN[4];
		3'b101 : OUT <= IN[5];
		3'b110 : OUT <= IN[6];
		3'b111 : OUT <= IN[7];		
	endcase	
end	
endmodule