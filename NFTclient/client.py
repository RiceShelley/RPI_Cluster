#client.py

import time
import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#sock.connect(('127.0.0.1', 9801))
#sock.connect(('hentainet3000.ddns.net', 9877))
sock.connect(('192.168.1.127', 9801))

def fileRecv(fileName):
    #binaryF <- holds the file in binary until writing time
    binaryF = b''
    #holds chunks of the file as they come in
    dataChunk = sock.recv(1000)
    #reads in chunk till server ends a msg with <EOF>
    while (dataChunk[-5:] != b'<EOF>'):
        binaryF += dataChunk
        dataChunk = sock.recv(1000)
    binaryF += dataChunk[:-5]
    #write binary to file
    newFile = open(fileName, "wb")
    newFile.write(binaryF)
    newFile.close()
    #let usr know file is done being retrived
    print("done!")


def sendFile(fileName):
    data = b''
    with open(fileName, 'rb') as f:
        data += f.read()
    sock.send(data + "<EOF>".encode())
    print(sock.recv(20).decode())

while (True):
    msg = input();
    if msg == "ls":
        sock.send("LIST".encode())
        back = sock.recv(1000)
        print(back)
    elif msg[:4] == "get ":
        req = "GET " + msg[4:]
        sock.send(req.encode())
        resp = sock.recv(10).decode()
        if resp == "FILEEXIST":
            print("File found!")
            fileRecv(msg[4:])
        elif resp == "!FILEEXIST":
            print("Unkown file in working directory '" + msg[4:] + "'")
    elif msg[:3] == "up ":
        req = "UPLOAD " + msg[3:]
        sock.send(req.encode())
        print('Enter file path: ')
        path = input()
        sendFile(path)
    elif msg[:6] == "mkDir ":
        sock.send(("MKDIR " + msg[6:]).encode())
    elif msg[:4] == "wDir":
        sock.send("GETWDIR".encode())
        print(sock.recv(256).decode())
    elif msg[:2] == "cd":
        sock.send(("CDIR " + msg[3:]).encode())
    elif msg[:4] == "root":
        sock.send("ROOTDIR".encode())
    elif msg[:3] == "rm ":
        sock.send(msg.encode());
    else:
        sock.send(msg.encode())
