import sys

files = []
for a in sys.argv[1:]:
    files.append(open(a))

lastsec = 0
line = ""
secs = []
vals = []
for f in files:
    for line in f:
        while(line.split()[0] == "#"):
            line = f.readline()
        sec, val = line.strip().split()
        secs.append(int(sec) + lastsec)
        vals.append(val)
    lastsec = secs[-1]

f = open("out.THee", "w")

for i in range(0,len(secs)):
    f.write(str(secs[i]) + "\t" + vals[i] + "\n")

