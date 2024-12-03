`timescale 1ns/1ns

module Decoder3To8 (IN, OUT);
input [2:0] IN;
output reg [7:0] OUT;

always @(*) 
begin
	OUT <= 8'b0000_0000;

	case (IN)
		3'b000 : OUT <= 8'b0000_0001;
		3'b001 : OUT <= 8'b0000_0010;
		3'b010 : OUT <= 8'b0000_0100;
		3'b011 : OUT <= 8'b0000_1000;
		3'b100 : OUT <= 8'b0001_0000;
		3'b101 : OUT <= 8'b0010_0000;
		3'b110 : OUT <= 8'b0100_0000;
		3'b111 : OUT <= 8'b1000_0000;		
	endcase	
end	
endmodule


