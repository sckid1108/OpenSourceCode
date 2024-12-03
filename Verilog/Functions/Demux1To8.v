/*
module Demux1To8( IN, OUT, SEL);
input  wire       IN;
input  wire[2:0] SEL;
output wire[7:0] OUT;

AND4 and_0 ( .A(IN), .B( ~SEL[2]), .C( ~SEL[1]), .D( ~SEL[0]), .Z(OUT[0]) ); //000
AND4 and_1 ( .A(IN), .B( ~SEL[2]), .C( ~SEL[1]), .D( SEL[0]) , .Z(OUT[1]) );
AND4 and_2 ( .A(IN), .B( ~SEL[2]), .C(  SEL[1]), .D( ~SEL[0]), .Z(OUT[2]) );
AND4 and_3 ( .A(IN), .B( ~SEL[2]), .C(  SEL[1]), .D( SEL[0]) , .Z(OUT[3]) );

AND4 and_4 ( .A(IN), .B( SEL[2]), .C( ~SEL[1]), .D( ~SEL[0]) , .Z(OUT[4]) );
AND4 and_5 ( .A(IN), .B( SEL[2]), .C( ~SEL[1]), .D(  SEL[0]) , .Z(OUT[5]) );
AND4 and_6 ( .A(IN), .B( SEL[2]), .C(  SEL[1]), .D( ~SEL[0]) , .Z(OUT[6]) );
AND4 and_7 ( .A(IN), .B( SEL[2]), .C(  SEL[1]), .D(  SEL[0]) , .Z(OUT[7]) );

endmodule
*/

module Demux1To8( IN, OUT, SEL);
input wire       IN;
input wire[2:0] SEL;
output reg[7:0] OUT;

always @(*) 
begin

	/*
	OUT[0] = (SEL == 0) ? IN : 1'bZ;
	OUT[1] = (SEL == 1) ? IN : 1'bZ;
	OUT[2] = (SEL == 2) ? IN : 1'bZ;
	OUT[3] = (SEL == 3) ? IN : 1'bZ;
	OUT[4] = (SEL == 4) ? IN : 1'bZ;
	OUT[5] = (SEL == 5) ? IN : 1'bZ;
	OUT[6] = (SEL == 6) ? IN : 1'bZ;
	OUT[7] = (SEL == 7) ? IN : 1'bZ;
	*/		
	
	OUT = 8'bZZZZZZZZ;
	
	case (SEL)
		3'b000 : OUT[0] = IN;
		3'b001 : OUT[1] = IN;
		3'b010 : OUT[2] = IN;
		3'b011 : OUT[3] = IN;
		
		3'b100 : OUT[4] = IN;
		3'b101 : OUT[5] = IN;
		3'b110 : OUT[6] = IN;
		3'b111 : OUT[7] = IN;
		default : OUT[0] = IN;
	endcase	
	
end	
endmodule

/*
module Demux1To8( IN, OUT, SEL);
input  wire       IN;
input  wire[2:0] SEL;
output wire[7:0] OUT;

assign OUT[0] = IN & ~SEL[0] & ~SEL[1] & ~SEL[2];
assign OUT[1] = IN &  SEL[0] & ~SEL[1] & ~SEL[2];
assign OUT[2] = IN & ~SEL[0] &  SEL[1] & ~SEL[2];
assign OUT[3] = IN &  SEL[0] &  SEL[1] & ~SEL[2];

assign OUT[4] = IN & ~SEL[0] & ~SEL[1] &  SEL[2];
assign OUT[5] = IN &  SEL[0] & ~SEL[1] &  SEL[2];
assign OUT[6] = IN & ~SEL[0] &  SEL[1] &  SEL[2];
assign OUT[7] = IN &  SEL[0] &  SEL[1] &  SEL[2];

endmodule
*/

/*
module Demux1To8( IN, OUT, SEL);
input wire       IN;
input wire[2:0] SEL;
output reg[7:0] OUT;

always @(*) 
begin
	case (SEL)
		3'b000 : OUT <= {1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0,   IN};
		3'b001 : OUT <= {1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0,   IN, 1'b0};
		3'b010 : OUT <= {1'b0, 1'b0, 1'b0, 1'b0, 1'b0,   IN, 1'b0, 1'b0};
		3'b011 : OUT <= {1'b0, 1'b0, 1'b0, 1'b0,   IN, 1'b0, 1'b0, 1'b0};
		
		3'b100 : OUT <= {1'b0, 1'b0, 1'b0,   IN, 1'b0, 1'b0, 1'b0, 1'b0};
		3'b101 : OUT <= {1'b0, 1'b0,   IN, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0};
		3'b110 : OUT <= {1'b0,   IN, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0};
		3'b111 : OUT <= {  IN, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0};				
	endcase	
end	
endmodule
*/