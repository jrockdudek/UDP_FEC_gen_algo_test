# UDP_FEC_gen_algo_test
I wrote this for college to learn a little more about genetic algorithms and networking. Uses a genetic algorithm to test a few different methods of forward error correction over UDP.

It worked by having a dummy ".wav" file. The client read that in, and sent a variable size of data(that evolved through the GA) with some vary basic form of forward error correction(again, evolved through the GA). The server side first recieved some information on what it was about to start recieving over a more reliable TCP connection. Then, it just listened over UDP for packets. Since it first got the data about the file and how it would be sent, it was able to keep track of if it was missing any packets. Unfortunately, I wasn't able to actually get good results with it.

I origanlly ran the server on one instance on an AWS server in Asia, and then ran the client from an instance in North America. The connection between the servers seemed to be too reliable though, and it never seemed to drop a packet over UDP.

Next, I tried setting up two routers with a laptop connected to each. One laptop ran the server, and the other, the client. Since I was using Wifi, I was finally able to see UDP packet loss. The problem with this setup was that it was on a college campus. So the interference was just constantly changing, giving me no consistent results.

If someone is interested in genetic algorithms, it may be a decent start at least. The GA code all seems to work, so that could be extracted, and played with in a different setting. It's not very generic, so it would have to be modified quite a bit though.

To compile the client:
gcc -pthread UDPGeneticAlgorithm.c -o gen_algo_test

To compile the server:
gcc UDPGASvr.c -o gen_algo_serv


It's currently hard coded to look at an ip address in "data.h"(127.0.0.1). So if you run the server in one terminal like(the port number is required since at one point, I planned to have the client test each generation concurrently, but that seemed to interfere with results, then I just never removed this leftover argument):
./gen_algo_serv 3001

And then just run the client with no arguments in another terminal. Obviously you won't see packet loss on just the loopback network, but it's there to just see. It also takes a while to run since I simulated as if you were streaming live audio. So it manually puts delays after sending audio data(the delay is how many milliseconds worth of data it sent in the last packet to make imitate real time).
