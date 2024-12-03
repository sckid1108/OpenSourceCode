`default_nettype none
`timescale 1ns / 1ns

module DFF ( D, Q, QN, CLK, RSTN ) /* synthesis GSR="ENABLED" */;
input wire D;
input wire CLK, RSTN;
output reg Q = 1'b0; 
output wire QN;

assign QN = ~Q;

// Combinational always block for the state machine
// Generates next state and output equations

always @(posedge CLK or negedge RSTN)
	if (RSTN == 1'b0)Q <= 1'b0; 
	else             Q <= D;			

endmodule