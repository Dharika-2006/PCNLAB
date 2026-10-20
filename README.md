for sockets:
gcc filename.c -pthread -o server (if thread is used)
gcc filename.c -o server
./server

for ns3 :
go to ns3 and save the file in scratch
run ./ns3 run scratch/filename
