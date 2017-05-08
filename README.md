# ThinkingPhilos

Simulation of the infamous Dijsktra's  ** Thinking Philosophers ** problem. using pthreads.

You need to have pthreads installed and linked to compile the code:

```
g++ philos.cpp -pthread -lncurses -std=c++11 -o philos
```

Philos.cpp is only the simulation code which can be modified to arrive at dead lock free and starvation free solutions.

nodeadlock.cpp is one of the possible dead lock free solutions and nostarve.cpp is one of the possible starvation free solutions.
