************************************************************************************************
Author:	Al Timofeyev
Date:	2/17/2019
Desc:	This is a CS 470 Server/Client program.
	This program was created in Linux Ubuntu 18.04 running
	in VirtualBox with gcc/g++ compiler.
************************************************************************************************

************************************************************************************************
BUGS:
1) Timeout Message needs to be properly printed when the server is not running or is terminated
   mid-operation. (Timeout message printed to client.)
2) Client (in manual mode) needs to be able to make multiple purchases instead of exiting after
   just one purchase.
3) Server needs to handle negative seating values during initialization/start. (Error management)
************************************************************************************************

Files needed for this program.
An ini text file (Ex: ini.txt, config.txt, asdf.txt, etc.).

The ini.txt file is to have the IP address, Port, and Timeout information for the client.
All three have to be on their own separate lines with spaces between every word.
Example:
IP = 127.0.21.2
Port = 4343
Timeout = 4

To compile both code files:
g++ AirfareServer.cpp -o server -pthread
g++ AirfareClient.cpp -o client -pthread

The server program accepts 2 parameters max for the rows and columns of seating map.
Two ways to run the server program.
1) Without Parameters:	./server
2) With Parameters:	./server 6 4

Here, 6 is the number of rows and 4 is the number of columns in the seating map.


The client program accepts 2 parameters max for the mode and the ini file.
Three ways to run the client program.
1) Without Parameters:	./client
2) With 1 Parameter:	./client manual
			./client automatic
3) With 2 Parameters:	./client manual ini.txt
			./client manual ini		(<----- This one was what I used on Linux Ubuntu.)