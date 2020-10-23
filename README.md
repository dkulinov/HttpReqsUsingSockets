# **Daniel Evans Systems Assignment**

This is a C++ program/CLI that uses POSIX sockets to make HTTP/1.1 GET REQUESTS.
Please note that this program will not work on a Windows computer unless a Linux VM is running on it.
This program will work on Linux/Unix and MacOS.

### **To Start Using the Program:**
1. Open up the terminal and go to the correct folder (if you're on Windows, don't forget to switch over to WSL)
1. Compile the source code found in the **systems.cpp** file using a C++ compiler you have installed on your computer. 
(Example for the g++ compiler: g++ systems.cpp -o filename)
1. Run the executable by typing ./filename
## ** You have 3 options: **
1. "./filename --help": will print out information on how to use the program
1. "./filename --url link": will make an HTTP/1.1 GET REQUEST to a provided link and will print out the response to the terminal
1. "./filename --url link --profile number": will make a specified number of HTTP/1.1 GET REQUESTS and print out some stats once it finishes. (This option does not print the response. If you'd like to see the response use the previous command) 

### ** PLEASE NOTE **
* Do not include "http://" or "www." in the link option. You may or may not provide the path. If no path is provided, the request will be made to the "/" path
* This only works for sites that accept HTTP requests. If the site only accepts HTTPS requests, a message with error 301 will be printed.
* You cannot use the --profile option without the --url option.
* You can use the short version of the options (-u, -p, -h)
