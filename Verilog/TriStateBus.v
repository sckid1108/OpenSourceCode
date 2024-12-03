
module TriStateBus(OUT, IN, SEL);
parameter WIDTH = 32;	
inout wire [WIDTH-1:0] OUT;
input wire [WIDTH-1:0] IN;
input wire SEL;

assign OUT = (SEL == 1'b1) ? IN : {WIDTH{1'bZ}};

endmodule