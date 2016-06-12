# vsmp_perf
Suite that allows to test SMP, vSMP and UP performance through the launch of test app in virtualized and non-virtualized environments.

This project was created as a part of Total Virtualization course at Innopolis University at 2016.

There are two applications:
- ThreadedController
- ThreadedAppV2

ThreadedAppV2 is an application that should run on target test environment (on some host or guest os). When it started, you will see at stdout list of interfaces, by which ThreadedController can reach the app.
When ThreadedController is launched, it tries to access ThreadedAppV2 by TCP and handle two experiments.

Launch syntax:
```
ThreadedAppV2.exe

ThreadedController.exe <ThreadedAppV2_IP>
ThreadedController.exe 192.168.0.10
```

## What are the experiments?
The test is formed of two experiments: run two threads in concurrent mode that tries to access shared variable. Threads are synchronized through the mutex. If the application runs in virtualized environment, then mutex handling will lead to significant performance degradation. 

The first experiment will be launched using different logical CPU (SMP or vSMP mode), and the second - the same logical CPU (UP mode).

Experiment details and results are described in slides: `SMP Virtualization Performance - Igor Egorov.pdf`

##References

This project uses sources of Practical C++ Sockets developed by Jeff_Donahoo ~at~ Baylor.edu, [link] (http://cs.baylor.edu/~donahoo/practical/CSockets/practical/)

