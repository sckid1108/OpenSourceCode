
// Moore FSM where the outputs are only a function of the current state
// outputs have a 1 cycle delay to settle through combinatorial logic (no glitches)
// 3 Always blocks for clarity
// 1 Hot Encoding
// State enumerations can go into PACKAGES to change encoding easily

module MooreFSM (
input logic ws, go, rstn, clk,
output logic rd, ds);

typedef enum logic[3:0] { IDLE  = 4'b0001, 
						  READ  = 4'b0010, 
			 			  DELAY = 4'b0100, 
						  DONE  = 4'b1000, 
						  XXXX  = 'x } eFSMState;

eFSMState cur_state, nxt_state;

// Output Assignment Logic
always_ff @(posedge clk, negedge rstn) 
	if (rstn == 1'b0) begin
		rd <= '0;
		ds <= '0;
	end
	else begin
		// default output assignments
		rd <= '0;
		ds <= '0;

		// state based output assignments
		case (nxt_state)
		IDLE  : ; // use defaults
		READ  : rd <= '1;
		DELAY : rd <= '1;
		DONE  : ds <= '1;	
		default : {rd, ds} <= 'x; // catch unassigned
		endcase
	end


// State Transition Logic
always_comb begin
	nxt_state = XXXX; // Better synthesis results Cummings FSM1 Page 18

	case (cur_state)
		IDLE  : if (go == 1'b1) nxt_state = READ;
				    else            nxt_state = IDLE;
		READ  :                 nxt_state = DELAY;
		DELAY : if (ws == 1'b0) nxt_state = DONE;
				    else            nxt_state = READ;
		DONE  :                 nxt_state = IDLE;
		default :               nxt_state = XXXX;
	endcase
end

// FSM State Register Inference Block
always_ff @(posedge clk, negedge rstn) 
	if (rstn == 1'b0) cur_state <= IDLE;
	else              cur_state <= nxt_state;

endmodule
