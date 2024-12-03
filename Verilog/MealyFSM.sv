
// Mealy FSM where the outputs are only a function of the current state and inputs
// ouputs have a 0 cycle delay to settle through combinatorial logic and can have glitches
// 2 Always blocks for clarity
// 1 Hot Encoding
// State ennumerations can go into PACKAGES to change encoding easily

module MealyFSM (
input logic ws, go, rstn, clk,
output logic rd, ds);

typedef enum logic[3:0] { IDLE  = 4'b0001, 
						  READ  = 4'b0010, 
			 			  DELAY = 4'b0100, 
						  DONE  = 4'b1000, 
						  XXXX  = 'x } eFSMState;

eFSMState cur_state, nxt_state;

// State Transition Logic and Output Assignments
always_comb begin
	nxt_state = XXXX; // Better synthesis results Cummings FSM1 Page 18
	
	// Output default assignments
	rd = '0;
	ds = '0; 
	
	case (cur_state)
    IDLE  : if (go == 1'b1) nxt_state = READ;
				else            	  nxt_state = IDLE;
		READ  : begin
									          nxt_state = DELAY;
					                  rd = '1;	
				end
		DELAY : begin
					  if (ws == 1'b0) nxt_state = DONE;
					  else            nxt_state = READ;					
					  rd = '1;
				end
		DONE  : begin
						nxt_state = IDLE;
					  ds = '1;
				end
		default : begin
					nxt_state = XXXX;
					
					ds = 'x;
					rd = 'x;					
				  end
	endcase
end

// FSM State Register Inference Block
always_ff @(posedge clk, negedge rstn) 
	if (rstn == 1'b0) cur_state <= IDLE;
	else              cur_state <= nxt_state;

endmodule
