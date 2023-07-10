import socket
import threading
import time
from resource import *

# Thread function
def run_client(host,port,max_time,senddata):

    # Create socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        totalbytes = 0

        # connect socket
        s.connect((host,port))

        # collect initial stats
        start_time = time.time()
        end_time = start_time + max_time
        start_usage = getrusage(RUSAGE_THREAD)

        # loop until time limit is reached
        while time.time() < end_time:

            # send the input data
            s.sendall(senddata)
            nbsent = len(senddata)

            # wait for all the data to be sent back
            bytes = 0
            while bytes < nbsent:
                data = s.recv(nbsent-bytes)
                bytes += len(data)
            totalbytes += bytes

        # get final stats
        finish_usage = getrusage(RUSAGE_THREAD)
        involuntary_switches = finish_usage.ru_nivcsw-start_usage.ru_nivcsw
        voluntary_switches = finish_usage.ru_nvcsw-start_usage.ru_nvcsw
        block_input = finish_usage.ru_inblock-start_usage.ru_inblock
        block_output= finish_usage.ru_oublock-start_usage.ru_oublock
        elapsed = time.time() - start_time
        send_rate = ( totalbytes * 1E-6 )/ elapsed

        # print stats
        print( "Client finished, rate:", send_rate, " MB/s  vol:", 
            voluntary_switches, " invol:", involuntary_switches, 
            " input:", block_input, " output:", block_output )

# default values
HOST  = "127.0.0.1" 
PORT  = 9500  
MAXTIME = 10.0
NUMTHREADS = 6
DATA = b"0" * 128 * 1024

# create threads
threads = []
for n in range(NUMTHREADS):
    th = threading.Thread(target=run_client,args=(HOST,PORT,MAXTIME,DATA))
    threads.append( th )

# start threads
for th in threads:
    th.start()

