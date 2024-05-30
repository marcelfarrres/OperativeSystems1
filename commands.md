- See the process ID of the blocking process in a certain port:
$ lsof -iTCP:8531 -sTCP:LISTEN


- Kill it:
$ kill -9 34515

