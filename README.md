# KM-to-UM-Injector
This project is a driver that can inject a shellcode given from the usermode client to a process by its pid.
The shellcode may not be longer than 64K, and is currently hardcoded in the usermode client.

I only tested it in windows 10 (1903).

Note: This was created as an educational exercise, I wouldn't use it in production code without testing 
