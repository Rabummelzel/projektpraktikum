from socket import *
import datetime as date
import threading
import time
from os.path import exists
from os import makedirs

#socket for incoming data from arduino
#address = (sys.argv[1].split(":")[0], int(sys.argv[1].split(":")[1]))
address = ('149.217.90.48', 5000)
client_socket = socket(AF_INET, SOCK_DGRAM) #Set Up the Socket
client_socket.settimeout(150)

#socket for listening to the controlling python script
server_port = 8080
server_socket = socket()
server_socket.bind(('127.0.0.1', server_port))
server_socket.listen(2)

log = open("tempreg.log", 'a', 1)
out = open("temp.dat", "a",1)

THepath = "/mnt/tritium/PyEnvDAQ/THeeFiles/"
Filename = "T_PID"

createtime = 150 #create THee files every ... seconds
temptime = 60 #time to get temp

def send(message):
    data = bytes(message, 'utf-8')
    client_socket.sendto(data, address)
    try:
        rec_data, addr = client_socket.recvfrom(2048)
        return rec_data.decode("utf-8")
    except:
        pass

def secDay(dateObj):
    return int((dateObj - dateObj.replace(hour = 0, minute = 0, second = 0, microsecond = 0)).total_seconds())

#threaded writer class for the Thee files
class THeeWriter(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.files = dict()
        self.alive = True

    def readFile(self,filepath):
        ret = ""
        if not exists(filepath):
            return ret
        f = open(filepath,"r+")
        f.readline()
        f.readline()
        f.readline()
        for line in f:
            ret += line
        return ret
        f.close()

    def header(self,day, length):
        return "# " + Filename + "\n" + "# " + day + "\n" + "# Number of points in this file: " + str(length)

    def write(self, dateObj, data):
        day = '{:%Y-%m-%d}'.format(dateObj)
        if day not in self.files:
            self.files[day] = str(secDay(dateObj)) + "\t" + data + "\n"
        else:
            self.files[day] += str(secDay(dateObj)) + "\t" + data + "\n"

    def writeNow(self):
        try:
            for day in self.files:
                year = day.split("-")[0]
                month = day.split("-")[1]
                path = filepath = THepath + year + "/month" + month + "/"
                if not exists(path):
                    makedirs(path)
                filepath = path + Filename + "_" + day + ".THee"
                data = self.readFile(filepath) + self.files[day]
                f = open(filepath, "w", 1)
                f.write(self.header(day,data.count("\n")) + "\n")
                f.write(data)
                f.close()
            self.files = dict()
        except BaseException as Error:
            log.write('{:%d.%m.%Y %H:%M:%S}'.format(date.datetime.now()) + " " + str(Error) + "\n\n")
            pass

    def run(self):
        i = 0
        while self.alive:
            if i == createtime:
                self.writeNow()
                i = 0
            time.sleep(1)
            i += 1

    def stop(self):
        self.writeNow()
        self.alive = False
        self.join()

#threaded class for receiving the temperature values and sending commands
class receiveTemp(threading.Thread):
    def __init__(self, writer):
        threading.Thread.__init__(self)
        self.writer = writer
        self.alive = True
        self.command = ""
        self.commandmessage = ""

    def run(self):
        timer = 0;
        try:
            while self.alive:
                time.sleep(1)
                if self.command != "":
                    self.commandmessage = send(self.command)
                if timer == temptime:
                    timer = 0
                    rec = send("g")
                    if rec == None:
                        timer = temptime
                        continue
                    lines = rec.split("\n")
                    for i in range(0, len(lines)):
                        dateObj = date.datetime.now() - date.timedelta(minutes = len(lines) - i - 1)
                        line = '{:%d.%m.%Y %H:%M:%S}'.format(dateObj) + " " + str(lines[i])
                        out.write(line + "\n")
                        self.writer.write(dateObj,str(lines[i].split()[1]))
                    #print(line)
                timer += 1;
        except BaseException as Error:
            log.write('{:%d.%m.%Y %H:%M:%S}'.format(date.datetime.now()) + " " + str(Error) + "\n\n")
            pass

    def clear(self):
        self.command = ""
        self.commandmessage = ""

    def stop(self):
        self.alive = False
        self.join()

thread = THeeWriter()
thread.start()
thread1 = receiveTemp(thread)
thread1.start()
command = ""
while(True):
    (controll, addr) = server_socket.accept()
    command = controll.recv(2048).decode("utf-8")
    message = ""
    if command == "stop":
        break
    if command == "g":
        message = "Temp is being measured"
    else:
        thread1.command = command
        while thread1.commandmessage == "":
            time.sleep(0.5)
        message = thread1.commandmessage
        thread1.clear()

        command = ""
    log.write('{:%d.%m.%Y %H:%M:%S}'.format(date.datetime.now()) + "\n")
    log.write(message + "\n\n")
    controll.send(bytes(message, "utf-8"))

thread1.stop()
thread.stop()
