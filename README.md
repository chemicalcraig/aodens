aodens
======

Calculate transition charges via atomic orbitals


Compile with

$ Make

and run with 

$./aodens.out <file>

where a sample input file has the following format

natoms: 39
donor log file:     pmimeth.out
donor movec file:   pmimethvecs.dat
excitations: rpa
root: 1

where the log file is an output file from the locally modified version of NWChem that supplies extended printing. The movec file is the MO vector file from an NWChem single point calculation and converted to human readable format using their mov2asc utility. The 'excitaton' field supplies the method of excitation used in the TDDFT calculation, either rpa or cis. The 'root' field tells the program which excited state information to extract.
