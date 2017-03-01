#client.py

import time
import socket
import _thread as thread
import sys

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((str(sys.argv[1]), 9800))

def wait_for_job(jobName, ip_addr, port):
    jobSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    jobSock.bind((ip_addr, int(port))) 
    jobSock.listen(1)
    conn, client_addr = jobSock.accept()
    conn.settimeout(1)
    rtn = ""
    temp = "nil"
    while temp != "":
        try:
            temp = conn.recv(100).decode() 
            rtn += temp
        except socket.error:
            break
    print(jobName + " output => " + rtn)
    conn.close()

while (True):
    msg = input()
    # run custom cmd on node ex. "do job echo Hello, World!" 
    if msg[:7] == "do job ":
        # Get ip
        ip = msg[7:]
        ip = ip[:ip.index(':')]
        ip = "127.0.0.1" if ip == "lh" else ip
        # Parse port
        port = msg[7:]
        port = port[(port.index(':') + 1):port.index(' ')]
        # Start server on a new thread to listen for end of job
        thread.start_new_thread(wait_for_job, ("custom job", ip, port, ))
        # Send nodeManager cmd
        sock.send(("DOJOB " + (ip + msg[msg.index(':'):])).encode())
    # ping node
    elif msg == "ping":
        sock.send("PING".encode())
        print(sock.recv(100).decode())
    # Flush socket
    elif msg == "flush":
        data = sock.recv(10)
        while data != '':
            data = sock.recv(10)
            print(data)
        print("socket flushed")

    # start app on node
    elif msg[:6] == "start ":
        # Get ip
        ip = msg[6:]
        ip = ip[:ip.index(':')]
        # Parse port
        port = msg[6:]
        port = port[(port.index(':') + 1):port.index(' ')]
        # Start server on a new thread to listen for end of job
        thread.start_new_thread(wait_for_job, ("start proc job", ip, port, ))
        # Send nodeManager cmd
        sock.send(msg.encode())
        print(sock.recv(100).decode())
    # kill app on node
    elif msg[:5] == "kill ":
        # Get ip
        ip = msg[5:]
        ip = ip[:ip.index(':')]
        # Parse port
        port = msg[5:]
        port = port[(port.index(':') + 1):port.index(' ')]
        # Start server on a new thread to listen for end of job
        thread.start_new_thread(wait_for_job, ("kill job", ip, port, ))
        # Send nodeManager cmd
        sock.send(msg.encode())
        print(sock.recv(100).decode())
    # Restart node 
    elif msg == "restart":
        sock.send(msg.encode())
        print(sock.recv(100).decode())
    elif msg == "help":
        sock.send(msg.encode())
        msg = sock.recv(10).decode()
        t_msg = "NULL";
        while t_msg != "":
            sock.settimeout(1.0)
            try:
                t_msg = sock.recv(10).decode()
            except socket.timeout:
                t_msg = ""
            msg += t_msg
        print(msg);
    else:
        sock.send(msg.encode())

