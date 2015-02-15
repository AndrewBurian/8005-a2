# C10K - Scalable Server Design Research


Installing From Sources
---------------
This program comes in three parts, all of which are available from this repository.
```shell
$ git clone http://github.com/AndrewBurian/8005-a2
```
Then in the resulting directories `client`, `server`, and `controller`, you can simply run
```shell
$ make
```

A System in 3 Parts
--------------
This project has 3 binaries, listed here, that make up the server, and the tools used to test it.

|Executable | Purpose
|------------ | --------------
|Server | This is the server itself and can be invoked to use either a forking, polling, or edge-triggered polling model for monitoring connections
|Controller | The controller is the master of the client program. It discovers any clients running on the broadcast-enabled LAN and slaves them to bombard the target server program with connections
|Client | The client spools up connections as commanded by the controller, then monitors response times from the server and reports the data back to the controller

Running
---------------
<h3>Server</h3>
The server requires one of either `--poll`, `--select`, or `--epoll` to be set.

|Options | Description
|------- | -----------
|`-p` <br/>`--poll` | Runs the server in polling mode. A new child process is created for each connection
|`-s`<br/>`--select` | Runs the server in level triggered kernel polling mode. Epoll is actually used over select for implementation but behavioral variations between this and select are negligible
|`-e`<br/>`--epoll` | Runs the server is edge-triggered kernel polling mode. Epoll is once again used with non-blocking sockets
|`-t`<br/>`--threads` | Only available with `--epoll`. Specifies a number of threads to run concurrently as Epoll in edge triggered mode can be called from muliple threads and remain thread safe

<h3>Controller</h3>
The controller requires all but `--output` or `--kill` to be set.

|Options | Description
|------- | -----------
|`-p`<br/>`--discover-port` | Specifies the port that the discovers will be broadcast on when searching for clients. Responses will be returned to this port + 1
|`-a`<br/>`--server` | Specifies the address of the target server in dotted decimal form
|`-s`<br/>`--data-size` | Specifies the amount of data that will be sent to the server per volley
|`-l`<br/>`--server-port` | Specifies the port the clients will connect to the server on
|`-i`<br/>`--increment` | Specifies how many connections should be added for each iteration of the test
|`-c`<br/>`--clients` | Specifies how many clients will be slaved at a maximum
|`-b`<br/>`--base-connects` | Specifies how many connections testing should start from
|`-v`<br/>`--vollies` | Specifies how many volleys of data will be sent for each iteration of testing
|`-o`<br/>`--output` | Specifies the output file to print results to. <br/> If not set, stdout is used
|`-k`<br/>`--kill` | If set, the controller will connect up to clients and order them to terminate, then exit.<br/>Requires `--clients` and `--discover-port`

<h3>Client</h3>
The client requires no arguments to be set

|Options | Description
|------- | -------------
|`-p`<br/>`--port` | Specifies the port that the client will listen on for incoming discovers from a controller<br/>If not set, 7002 is the default

Results
-----------------------
Not yet gathered.

License
-----------------
See [License](LICENSE)
