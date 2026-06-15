## Program Execution Test
```
root@codespaces-010fd0:/workspaces/vsd-riscv2/samples# spike pk sum1ton.o
bbl loader
Sum from 1 to 9 is 45 
```
## Where is the RISC-V program located in the vsd-riscv2 repository?
* at Samples directory

## How is the program compiled and loaded into memory?
* Compiled through riscv64-unknown-elf-gcc compiler and creates the object file
* Spike riscv simulator, takes the object file and load into the memory
  
## How does the RISC-V core access memory and memory-mapped IO?
* RISC-V core accesses memory and memory-mapped IO through instructions provided at the execution level. Moreover, there is no special instruction for accessing memory-mapped IO.
* The load, store instruction used for accessing memory, can also be used for MMIO
  
## Where would a new FPGA IP block logically integrate in this system?
* In this system, we can use memory-mapped IO to integrate and communicate with IP
