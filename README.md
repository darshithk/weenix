# weenix
Toy Linux Kernel Development

##### Download and Setup
	% gunzip -c weenix-assignment-1.0.2.tar.gz | \
        tar xvf -
	% cd weenix-assignment-1.0.2/weenix
	% make clean
	% make
	% ./weenix -n

##### Compilation and Configuration
* Config.mk controls what gets compiles and configured into the kernel
	- for PROCS, use the original Config.mk
	- for VFS, set DRIVERS and VFS to 1
	- for VM, set DRIVERS, VFS, S5FS, VM, and DYNAMIC to 1

* Modify Config.mk first, then do:


		% make clean
    	% make
    	% ./weenix -n