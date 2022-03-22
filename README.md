# Clinic
Unix Semaphore,Mutex,and Multithreading Project
Created as a part of "Operating Systems" course in my 3rd year.
This program shows the use of semaphore,threads,multithreading in UNIX based environment and C

Shortly:There is a clinic with 3 dentist, each of them can only serve one client.
we keep a queue of clients using LinkedLists.

In a "Room" of 10 clients, there is a sofa that can sit "only" 4 clients, so thus we have a queue of clients who wait to be sitted,
The sitting on the couch depens on the arrival time of the client, so does the serve time of the dentits to a certain clients.

a dentits can serve only 1 client at a time, also only 1 client can pay for the service at a time.


![image](https://user-images.githubusercontent.com/88554020/159493440-334b740b-c7ec-47e4-a0e7-5855cd56e9eb.png)

