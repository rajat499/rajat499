import socket
import threading
import time
import os
from resource import *

# thread to serve each connection
def server_thread(conn):
    # Loop receiving data until disconnected

    start_time = time.time()
    start_usage = getrusage(RUSAGE_THREAD)
    totalbytes = 0 
    while True:
        try:
            data = conn.recv(4*1024*1024)
            totalbytes += len(data)
            if not data:
                break
            conn.sendall(data)
        except Exception as e:
            print(e)
            break
        
    # get final stats
    finish_usage = getrusage(RUSAGE_THREAD)
    involuntary_switches = finish_usage.ru_nivcsw-start_usage.ru_nivcsw
    voluntary_switches = finish_usage.ru_nvcsw-start_usage.ru_nvcsw
    elapsed = time.time() - start_time
    send_rate = ( totalbytes * 1E-6 )/ elapsed

    # print stats
    print( "Thread finished, rate:", send_rate, " MB/s  vol:", 
           voluntary_switches, " invol:", involuntary_switches )
           

def run_server( host, port ):

    # create listen TCP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

        # bind and listen
        s.bind((host, port))
        s.listen()

        # not necessary but here for tests
        s.settimeout(0.5)

        # loop forever
        print( "Listening...")
        while True:

            # wait for connection
            try:
                conn, addr = s.accept()
            except socket.timeout:
                continue
            except:
                raise
            print(f"New connection from {addr}")

            # Start thread to serve connection
            th = threading.Thread(target=server_thread,args=(conn,))
            th.start()

HOST = "127.0.0.1" 
PORT = 9500  
run_server( HOST, PORT )

