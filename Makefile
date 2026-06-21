.PHONY: all bai01 bai02 bai03 clean

all: bai01 bai02 bai03

bai01:
	$(MAKE) -C bai01_shell_system_admin

bai02:
	$(MAKE) -C bai02_process_file_socket_network

bai03:
	$(MAKE) -C bai03_kernel_module_integration

clean:
	$(MAKE) -C bai01_shell_system_admin clean
	$(MAKE) -C bai02_process_file_socket_network clean
	$(MAKE) -C bai03_kernel_module_integration clean
