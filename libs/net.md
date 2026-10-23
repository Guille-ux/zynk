# Net Library for `Zynk`
*(Only for POSIX)*

Net adds a new global object called `Net` that contains the following functions:

- `socket()`: Creates a new socket

- `getip(hostname)`: Returns the ip of the hostname

- `connect(socket_fd ip_string port)`: Connects with an ipv4 address at an specified address

- `send(socket_fd message)`: Sends a string

- `recv(socket_fd blocking)`: Recives information in form of string

- `bind(socket_fd port)`: Binds the socket to a port

- `listen(socket_fd max_cons)`: Makes the server start listening at the binded port

- `accept(socket_fd)`: Returns the `fd` of the connection with the client

- `close(socket_fd)`: Closes a connection

Example:

```zynk
include("libs/net.so")

let socket = Net:socket()

let hostname = "www.google.com"

let ip = Net:getip(hostname)

let port = 80
let request = "GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n"

Net:connect(socket ip port)
Net:send(socket request)

let text = "\n"
let new
let end
do {
	new = Net:recv(socket 1)
	text = text + new
	end = len(text)
	echo new
	if index(text end)=="\n" and index(text end-1) == "\r" and index(text end-2) == "\n" and index(text end-3)=="\r" or len(new)<10 then
		echo "CONNECTION ENDED"
		Net:close(socket)
		break
	fi

} while 1
```
