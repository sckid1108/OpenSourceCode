// Using a logic block to implement the decoder. 
// Inside the always_comb block, we first initialize all output bits to 0 by assigning the out signal to '0. 
// Then, we set the corresponding decoded bit to 1 by accessing the output bit at the index specified by the 
// input signal in and assigning it the value of 1'b1.
module DecoderEnN ( IN, EN, OUT );
parameter EncodeWidth = 4;
parameter DecodeWidth = 2 ** EncodeWidth;
input logic [EncodeWidth-1:0] IN;
output logic [DecodeWidth-1:0] OUT;
input logic EN;

always_comb begin
    // Initialize all output bits to 0
    OUT = '0;
    
    // Set the corresponding decoded bit to 1
	if (EN == 1'b1)
		OUT[IN] = 1'b1;
end

endmodule
