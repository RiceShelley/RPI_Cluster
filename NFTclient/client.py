# client.py <- client for use with NFT fileServer written by rootieDev@gmail.com
# params python3.x server_ip server_port

import time
import socket
import sys
import os

# Gather cmd line args
serv_ip = str(sys.argv[1])
serv_port = int(sys.argv[2])

# connect to serv
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((serv_ip, serv_port))

def fileRecv(fileName):
    #binaryF <- holds the file in binary until writing time
    binaryF = b''
    #holds chunks of the file as they come in
    dataChunk = sock.recv(1000)
    # Keep track of bytes read
    bRead = 0
    #reads in chunk till server ends a msg with <EOF>
    while (dataChunk[-5:] != b'<EOF>'):
        binaryF += dataChunk
        dataChunk = sock.recv(1000)
        bRead += 1000
        print("bytes read[" + str(bRead) + "]", end='\r')
    binaryF += dataChunk[:-5]
    print("bytes read[" + str(bRead) + "]")
    #write binary to file
    print("writing file to local disc")
    newFile = open(fileName, "wb")
    newFile.write(binaryF)
    newFile.close()
    #let usr know file is done being retrived
    print("done!")


def sendFile(fileName):
    f = open(fileName, 'rb')
    size = int(os.path.getsize(fileName))
    bytesRead = 0
    loadingBar = ""
    print(size)
    while (True):
        binD = f.read(700)
        if binD == b'':
            break
        bytesRead += 700
        loadingBar = '#' * int((bytesRead / size) * 100) + '-' * (100 - int((bytesRead / size) * 100))
        print("progress[" + loadingBar + "]", end='\r')
        sock.send(binD)
        time.sleep(.25)
    time.sleep(1)
    print("progress[" + loadingBar + "]")
    sock.send("<EOF>".encode())
    sock.settimeout(30)
    try:
        print(sock.recv(20).decode())
    except socket.error:
        print("WARNING: did not get file sent cofirmation!!!")
    sock.settimeout(1)
    


while (True):
    msg = input();
    if msg == "ls":
        sock.send("LIST".encode())
        f_back = ""
        back = "temp"
        while back != ' ':
            sock.settimeout(1)
            try:
                try:
                    back = sock.recv(10).decode()
                except UnicodeDecodeError:
                    f_back += "<NON UTF-8 FILE NAME>" 
                    continue
                f_back += back
            except socket.error:
                break
        print(f_back)
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
        print('Enter file path: ')
        path = input()
        if os.path.isfile(path) == False:
            print("file does not exist")
            continue
        req = "UPLOAD " + msg[3:]
        sock.send(req.encode())
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
    elif msg[:8] == "setWDir ":
        print("wat")
        sock.send(("SETWDIR " + msg[8:]).encode())
    else:
        sock.send(msg.encode())
