// D-FlipFlop Synchronous Active Low Reset and Synchronous Clock Enable

`default_nettype none
`timescale 1ns / 1ns

module DFFEN ( D, Q, QN, CE, CLK, RSTN ) /* synthesis GSR="ENABLED" */;
input wire D;
input wire CE, CLK, RSTN;
output reg Q;
output wire QN;

// Combinational always block for the state machine
// Generates next state and output equations

assign QN = ~Q;

always @(posedge CLK or negedge RSTN)
	if (RSTN == 1'b0)      Q <= 1'b0; 
	else if ( CE == 1'b1 ) Q <= D;			
		 else              Q <= Q;	

endmodule		 

