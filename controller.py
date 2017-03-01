import socket
import sys

inp = sys.argv[1]
s = socket.socket()
s.connect(('127.0.0.1', 8080))
s.send(bytes(inp, "utf-8"))
rec_data = s.recv(2048)
print(rec_data.decode("utf-8"))

