import socket
import _thread as thread
import time

# class for handling node connections 
class connHandle:

    usedPorts = []

    def __init__(self, sock_wrapper):
        self.sock_wrapper = sock_wrapper
        self.sock = self.sock_wrapper.get_sock()
        self.ip_addr = self.sock_wrapper.get_ip_addr()

    # Find unused port
    def assign_port(self, ip_addr, port):
        # see if port is unused
        for u_port in connHandle.usedPorts:
            if u_port == port:
                return "redo"
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.bind((ip_addr, int(port)))
        except socket.error: 
            sock.close()
            return "redo"
        sock.close()
        return str(port)

    # wait for job finished response from server
    def wait_for_job(self, jobName, ip_addr, port):
        WFJ_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            WFJ_sock.bind((ip_addr, int(port)))
        except socket.error:
            print("ERROR ON [" + ip_addr + "] failed to bind sock on local client")
        connHandle.usedPorts.append(port)
        WFJ_sock.listen(5)
        conn, client_addr = WFJ_sock.accept()
        print(jobName + " [FINISHED] ON PORT [" + str(port) + "]" + " IP [" + self.sock_wrapper.get_ip_addr() + "]")
        conn.close()
        connHandle.usedPorts.remove(port)


    # Do "job" on server. "jobs" r executing cmd in terminal remotely 
    def do_job(self, job):
        # Parse out ip
        job = job[7:]
        ip = job[:job.index(':')]
        # Parse port  
        port = job[(job.index(':') + 1):job.index(' ')]
        tickPort = port
        while True:
            port = self.assign_port(ip, tickPort)
            if port != "redo":
                break
            tickPort = str(int(tickPort) + 1)
        port = tickPort
        # Start server on new thread to liten for end of job
        thread.start_new_thread(self.wait_for_job, ("custom job", ip, port, ))
        # Stitch in new port 
        job = job.replace(job[(job.index(':') + 1):job.index(' ')], str(port))
        self.sock.send(("DOJOB " + job).encode())


    def start(self, cmd):
        # sub str "start " off of cmd
        cmd = cmd[6:]
        # Get ip
        ip = cmd[:cmd.index(':')]
        # Parse port
        port = cmd[(cmd.index(':') + 1):cmd.index(' ')]
        port = self.assign_port(ip, port)
        # Start server on a new tread to listen for end of job 
        thread.start_new_thread(self.wait_for_job, ("start procc job", ip, port, ))
        # Stitch in new port 
        cmd = cmd.replace(cmd[(cmd.index(':') + 1):cmd.index(' ')], str(port))
        # Send node manager cmd
        self.sock.send(("start " + cmd).encode())
        print(self.sock.recv(100).decode())

    def kill(self, cmd):
        # Get ip
        cmd = cmd[5:]
        ip = cmd[:cmd.index(':')]
        # Parse port
        port = cmd[(cmd.index(':') + 1):cmd.index(' ')]
        # Start server on a new thread to listen for end of job
        thread.start_new_thread(self.wait_for_job, ("kill job", ip, port, ))
        # Stitch in new port 
        cmd = cmd.replace(cmd[(cmd.index(':') + 1):cmd.index(' ')], str(port))
        # Send nodeManager cmd
        self.sock.send(("kill " + cmd).encode())
        print(self.sock.recv(100).decode())

    def updateGOL(self):
        self.do_job("do job 192.168.1.174:7343 cd userBin/storage && rm -r gameOfLifeCluster")
        time.sleep(5)
        self.do_job("do job 192.168.1.174:2832 cd userBin/storage && git clone https://github.com/rootieDev/gameOfLifeCluster.git")
        time.sleep(15)
        self.do_job("do job 192.168.1.174:8544 cd userBin/storage/gameOfLifeCluster && gcc Main.c")
    

    def step(self, input):
        # Run custom cmd on node ex. "do job echo Hello, World!"
        if input[:7] == "do job ":
            self.do_job(input)
        # Ping node
        elif input == "ping":
            self.sock.send("PING".encode())
            try:
                print(self.sock_wrapper.get_ip_addr() + " <RES> " + self.sock.recv(100).decode())
            except socket.error:
                print(self.sock_wrapper.get_ip_addr() + " [UN RES] -- NO RES")
        # Start app on node 
        elif input[:6] == "start ":
            self.start(input)
        # Kill app on node
        elif input[:5] == "kill ":
            self.kill(input)
        # Restart node
        elif input == "restart":
            self.sock.send(input.encode())
            try:
                print(self.sock_wrapper.get_ip_addr() + " <RES> " + self.sock.recv(100).decode())
            except:
                print(self.sock_wrapper.get_ip_addr() + " [UN RES] -- NO RES")
        # Shutdown node
        elif input == "shutdown":
            self.sock.send("shutdown".encode())
        # update game of life software
        elif input == "update GOL":
            thread.start_new_thread(self.updateGOL, ())
