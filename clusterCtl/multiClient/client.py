#client.py 
import time
import socket
import _thread as thread
from time import sleep
import connHandle

# socket wrapper class
class sockW:
    def __init__ (self, sock, ipAddr):
        self.sock = sock
        self.ipAddr = ipAddr

    def get_ip_addr(self):
        return self.ipAddr

    def get_sock(self):
        return self.sock

# List of addr to connect to 
addr = ['192.168.1.100',
        '192.168.1.101', 
        '192.168.1.102', 
        '192.168.1.103', 
        '192.168.1.104', 
        '192.168.1.105', 
        '192.168.1.106', 
        '192.168.1.107', 
        '192.168.1.108', 
        '192.168.1.109', 
        '192.168.1.110', 
        '192.168.1.111']

#addr = ['192.168.1.100']

# List of connHandle objects
conn = []

# connect to all addr
for i in addr:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(1)
    try: 
        sock.connect((i, 9800))
        print(i + " + [OK]")
        sock_w = sockW(sock, i)
        conn_H = connHandle.connHandle(sock_w) 
        conn.append(conn_H)
    except socket.error:
        print(i + " + [BAD]")
        continue

while (True):
    msg = input()
    if msg[:4] == "all ":
        for conn_H in conn:
            conn_H.step(msg[4:])
    elif msg[:5] == "allH ":
        for i in range(int(len(conn) / 2), len(conn)):
            conn[i].step(msg[5:]) 
    elif msg[:5] == "allL ":
        for i in range(0, int(len(conn) / 2)):
            conn[i].step(msg[5:])
    elif msg == "ls":
        print("\tCLIENT LIST\n<------------------------->\n")
        for conn_H in conn:
       	    print("| CLIENT: [" + conn_H.ip_addr + "] |\n")
        print("<------------------------->")
    elif msg == "help":
        client = conn[0]
        client.sock.send("help".encode())
        print(client.sock.recv(1000).decode())
    elif msg[:3] == "ip ":
        msg = msg[3:]
        client_ip = msg[:msg.index(' ')]
        msg = msg[msg.index(' ') + 1:]
        for conn_H in conn:
            if conn_H.ip_addr == client_ip:
                conn_H.step(msg)
    else:
        print("UNKOWN CMD!!!")
