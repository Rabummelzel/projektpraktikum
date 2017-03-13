import sys

files = []
for a in sys.argv[1:]:
    files.append(open(a))

def addThee(files):
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
        if secs[-1] < secs[-2]:
            lastsec = secs[-2]
            secs[-2] += lastsec
        else: lastsec = secs[-1]
    return (secs, vals)

f = open("out.THee", "w")

secs, vals = addThee(files)
for i in range(0,len(secs)):
    f.write(str(secs[i]) + "\t" + vals[i] + "\n")
