# ThreadBoosterDriver

## Overview 

The ThreadBooster Driver is a simple user/kernel mode interaction driver which enables a user to enhance the priority of a thread (identified by a Thread ID) anywhere between the values of 0 and 31. 
Typically, this has to be done with SetPriorityClass and SetThreadPriority functions. However, by calling the client code executable (ThreadBoosterClient binary) with a thread id and priority value, 
this will be done automatically through the driver. 

## Usage 

Assuming you have built the solution and have access to a .sys file and ThreadBoosterClient binary, 

### Insert the driver using the sc.exe service in elevated mode 

```bash
sc create booster type= kernel binPath= <path to .sys file> # booster is the name we are assigning, this can be arbitrary
```

### Loading the driver module 

```bash
sc start booster 
```

Once started, run the executable 
```bash
ThreadBoosterClient <tid> <priority> 
```

### Unloading the driver module 
``` bash 
sc stop booster
```


