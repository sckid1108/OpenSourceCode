`default_nettype none
`timescale 1ns / 1ns

module DFF ( D, Q, QN, CLK, RSTN ) /* synthesis GSR="ENABLED" */;
input wire D;
input wire CLK, RSTN;
output reg  Q; 
output wire QN;
parameter INITIAL_VALUE = 1'b0;

assign QN = ~Q;

// Combinational always block for the state machine
// Generates next state and output equations

always @(posedge CLK or negedge RSTN)
	Q <= (RSTN == 1'b0) ? INITIAL_VALUE : D;
endmodule