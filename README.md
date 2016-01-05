Chatter
=======
is a chat program consisting of a client and server, written in C with sockets, pthreads, and ncurses. Right now I'm working on implementing encryption!
Usage
-----
First, you'll need to know the external IPv4 address or hostname of the computer on which you want to run the server. If you don't already know the hostname, you
can run

    curl ifconfig.me
Now to run the server, compile it first with

    make server
Then run

    ./server PORTNO
where PORTNO is the number of the port on which to accept connections.

To run the client, first compile it with

    make client
Then run

    ./client ADDRESS PORTNO
where ADDRESS is the IPv4 address or hostname you figured out before running the server, and PORTNO is the same number you supplied to run the server. You can run the client and server on the same computer if you want, just give "localhost" as the address when running the client.

Once you've loaded up the client and connected to the server, you can change your nick from the default ('Anon') to whatever you want with the command

    /nick NEWNICK

Exit the program with

    /exit
