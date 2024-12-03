
// D - Data In
// Q - Data Out
// RE - Rising Edge Detected
// FE - Falling Edge Detected
// CLK - Clock
// RSTN - Reset Active Low
module EdgeCatcher( D, Q, RE, FE, CLK, RSTN );
	
input wire D,CLK,RSTN;
output wire Q, RE,FE;

wire w1,w2;

DFF dd1( .D(D),  .Q(w1), .QN(), .CLK(CLK), .RSTN(RSTN) );DFF dd2( .D(w1), .Q(w2), .QN(), .CLK(CLK), .RSTN(RSTN) );
DFF dd3( .D(w2), .Q(Q) , .QN(), .CLK(CLK), .RSTN(RSTN) );
	
assign RE = ~Q &  w2;	
assign FE =  Q & ~w2;

endmodule