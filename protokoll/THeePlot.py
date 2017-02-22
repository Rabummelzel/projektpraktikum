from fitting import Fit
import numpy as np
def THeePlot(filepath, calib, res):
    dat = open(filepath)
    dat.readline()
    dat.readline()
    dat.readline()
    x = []
    y = []
    i = 0
    for line in dat:
        if i % res == 0:
            a,b = line.split()
            x.append(float(a))
            y.append(float(b))
        i += 1
    plot = Fit(np.array(x)/60/60,np.vectorize(calib)(np.array(y)))
    return plot