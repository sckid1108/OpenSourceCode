# Verilog Coding References and Suggestions

I recommend SystemVerilog for everything. 
Note: Lattice Diamond will support System Verilog if you use the Synopsys Synplify synthesis tool instead of LSE. 

[From SNUG 2019 - FSM Design and Synthesis using System Verilog Part I - Cummings](https://github.com/sckid1108/OpenSourceCode/blob/main/Verilog/Documentation/CummingsSNUG2019SV_FSM1.pdf)

SystemVerilog coding styles help facilitate proper simulation and understanding of the FSM coding 
styles. Below is a list of items that were identified as checklist items in this paper. After coding your 
FSM design, we recommend compare your code against the items in this checklist. 

<b>Checklist item:</b> Engineers should generally declare all FSM ports and all FSM internal signals to be 
of type logic. <i>Note that when you use logic for a port and the preprocessor macro `default_nettype none
you need to  declare the port type either wire or reg otherwise the synthesizer could give you an error.
-MM </i>

<b>Checklist item:</b> Where ever possible, use the SystemVerilog '0 / '1 / 'x to make assignments. 

<b>Checklist item:</b> Declare all ports to be of type logic. Extra points for listing outputs followed by 
inputs followed by control inputs. Extra points for grouping multiple signals into each output and 
input port list declaration. 

<b>Checklist item:</b> Declare the state and next variables using enumerated types. Add an XX or XXX
state to help debug the design and to help optimize the synthesized result. 

<b>Checklist item:</b> Use always_ff and always_comb procedures to infer clocked and combinatorial 
logic. Do not use the older Verilog always procedures. 

<b>Checklist item:</b> Declare the state register using just 3 lines of code and place it at the top of the design 
after the enumerated type declaration. 

<b>Checklist item:</b>  Use  a  default  next='x  or  next=state  default  assignment  at  the  top  of  the 
always_comb procedure. Start by first using the next='x style to help debug the FSM design. 

<b>Checklist item:</b> Extra points for using next instead of using nextstate in the FSM design, but both 
work fine. 

<b>Checklist item:</b> Extra points for placing the next assignments in a neat column in the FSM RTL code. 

<b>Checklist item:</b> Place default output assignments before the output‐assignment case statement and 
then to update the appropriate outputs for each state where the outputs change. 

When  comparing  the  coding  effort  for  the  four  coding  styles  (1‐always  block,  2‐always  block,  
3‐always block and 4‐always block) the 1‐always block coding style is very verbose and increases in 
size quickly with more states and outputs. For the larger prep4 design, the coding effort was more 
than twice as hard as coding the 3‐always block style.  

It was also interesting to note that the 3‐always block style frequently required less coding effort 
than the 2‐always block style with combinatorial outputs.  

The 1‐always block and 4‐always block coding styles typically gave slightly better synthesis area and 
timing performance over the 3‐always block style, but we recommend using the efficient 3‐always 
block style and then converting it to a 4‐always block style if slightly better synthesis performance is 
needed. 
