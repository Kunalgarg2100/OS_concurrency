PROBLEM 2: EVM PROBLEM
======================

There are three structs defined.
1. **EVM** has:
 - idx of EVM,
 - thread id of thread corresponding to that EVM 
 - corresponding booth num
 - no of slots in EVM 
 - boolean flag for its working(1 -> working).

2. **VOTER** has:
 - idx of Voter
 - status(NEW_VOTER,WAITING_VOTER,ASSIGNED_VOTER,COMPLETED_VOTER)
 - thread id of thread corresponding to that voter
 - corresponding booth num
 - corresponding evm

3. **BOOTH** has:
 - idx of Booth
 - thread id of thread corresponding to that booth
 - no of voters
 - max num of slots in evm
 - num of voters voted
 - num of evms
 - mutex to assign voter and corresponding conditions.
 - all the voters
 - all the evms

INITIALIZING FUNCTIONS
----------------------
they creates corresponding structs for each of them and initiailses them to default values

BOOTH THREAD:
-------------
It initializes the voters and EVMs
Then it creates threads for voters and EVMs
Then it joins threads for voters and EVMs

VOTER THREAD:
-------------
Voter waits for a EVM slot to be free for it.
On gaining it it votes.
The it signals that it is free so someone else can occupy it.

EVM THREAD:
-----------
EVM contains slots i random number form 1 to 10
EVMs waits for booth to start.
Then it waits for voters to come. 
It starts accepting votes when it is completely filled.
After all voters are done it declares itself to be free.

RUNNING IT:
-----------
1. gcc evm.c -lpthread
2. ./a.out
3. Input number of booths
4. For each booth input number of EVMs, number of Voters.
