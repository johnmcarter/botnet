# botnet
Botnet for IoT Ecosystem - Relies on IoT Malware repo for malware attack implementations

#### Compilation
The user can simply run 
```
make
```
on the command line to compile the server and bot code. This will create two executables: server and bot.

#### Server
To start the newly created botnet command and controll (C&C) server, run
```
./server <port number>
```

#### Bot
To start a new bot on a client, run
```
./bot <IP of server> <port on server>
```

##### Information
The bots connect to the C&C server to receive user-defined commands to run on bot-infected devices. The bots are designed to interact with the iot_malware repo to carry out specific attacks, such as a keylogger or TCP Reset attack. The server and bot implementations are both multi-threaded using POSIX threads, which allows for more action to happen concurrently. Specifically, the server can connect to and be communicating with multiple bots on different machines and send potentially different commands to each at the same time. Additionally, each bot can receive multiple commands from the server at the same time and spawn a new thread to handle each one, rather than blocking until the first command received has completed. 