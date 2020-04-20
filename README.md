# Ping-CLI (Linux)
A small Ping CLI application for Linux. This CLI app accepts a hostname or an IP address as its argument, then sends ICMP "echo requests" in a loop to the target while receiving "echo reply" messages. It reports loss and RTT times for each sent message.

For Linux machines only.

To use:
Run the makefile

Run the program (./ping) with root permissions 

Ex: sudo ./ping www.google.com
