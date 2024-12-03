
module OneShotPulseWidth(OUT, OUTN, RSTN, CLK) /* synthesis GSR="ENABLED" */;
parameter PULSEWIDTH = 8; // PULSEWIDTH * CLOCKPERIOD
localparam WIDTH = $clog2(PULSEWIDTH+1) - 1;
output reg OUT, OUTN;
input wire RSTN, CLK;


reg [WIDTH:0] qd;
initial qd = 0;

always @(posedge CLK or negedge RSTN) begin
	if (RSTN == 1'b0) begin
		qd   <= '0;
		OUT  <= 1'b0; 
		OUTN <= 1'b1;
	end
	else begin
		if (qd < PULSEWIDTH) begin
			OUT  <= 1'b1; 
			OUTN <= 1'b0;
			qd   <= qd + 1'b1;
		end
		else begin
			OUT  <= 1'b0; 
			OUTN <= 1'b1;
		end		
	end	
end

endmodule