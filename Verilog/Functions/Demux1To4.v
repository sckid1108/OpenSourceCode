`timescale 1ns/1ns

module Demux1To4( IN, OUT, SEL);
input  wire       IN;
input  wire[1:0] SEL;
output wire[3:0] OUT;

AND3 and0 ( .A(IN), .B(~SEL[1]), .C(~SEL[0]), .Z(OUT[0]) );
AND3 and1 ( .A(IN), .B(~SEL[1]), .C( SEL[0]), .Z(OUT[1]) );
AND3 and2 ( .A(IN), .B( SEL[1]), .C(~SEL[0]), .Z(OUT[2]) );
AND3 and3 ( .A(IN), .B( SEL[1]), .C( SEL[0]), .Z(OUT[3]) );

endmodule


/*
module Demux4To1( IN, OUT, SEL);
input wire       IN;
input wire[1:0] SEL;
output reg[2:0] OUT;

always @(*) 
begin
	case (SEL)
		2'b00 : OUT[0] <= {1'b0, 1'b0, 1'b0, IN};
		2'b01 : OUT[1] <= {1'b0, 1'b0, IN, 1'b0};
		2'b10 : OUT[2] <= {1'b0, IN, 1'b0, 1'b0};
		2'b11 : OUT[3] <= {IN, 1'b0, 1'b0, 1'b0};
	endcase	
end	
endmodule
*/