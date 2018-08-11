PROBLEM 1: GASSTATION PROBLEM
======================

There are 2 types of threads:
1. Threads for Attendees
2. Threads for Cars

Two Pipes are used:
1. Pipe between car and  attendee
2. Pipe between car and acceptPayment 

Functions used:
enterStation()
--------------
The function waits for the signal of maxcapacity,i.e if station is already full or not
When it is signalled by maxcapacity the car enters the station

waitInLine(int CarId)
--------------
Car has entered the station and this function is called if all gas stations are filled, then car will wait in a queue. It decrements value of variable spaceInqueue securely and car leaves the
station when signalled by totalGaspumps, and increments the vlaue of spaceInqueue

goToPump(int CarId)
---------------
Signalled by totalGaspumps car goes to the pump and decrements the value stations available securely and then writes in the pipe.

leavePump(int CarId,int i)
---------------
After servicing call leaves the station

pay(int CarId)
-------------
Car signals that it ready for payment

exitStation(int CarId)
-----------------
after payment is done car leaves the station

carMaker()
---------
Used to make cars after a random time

acceptPayment()
-------------
After car is done with servicing and ready with payment this function is called, It waits for the attendee to come and collect the payment.
After attendee accepts the payment it signals receipt for the car


RUNNING IT:
-----------
1. gcc station.c -lpthread
2. ./a.out