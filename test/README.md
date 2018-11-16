ta/tb/tc: start them at the same time in different teminals, they are communicating to each other

t1: just listening and converting messages to upper case

t2: sending message to t1

t3: sending message to global message bus

t4: subscribing to global message bus, printing all messages received on the bus
