import plotly.graph_objs as go
from scipy import optimize as opt
import inspect
import numpy as np
import scipy as sc

class Fit:
    def __init__(self,x_data, y_data, y_error=None):
        self.x_data  = x_data
        self.y_data  = y_data
        self.y_error = y_error
        self.funs   = []
        self.popts  = []
        self.pcovs  = []
        self.dofs   = []
        self.chisqs = []
        self.bounds = []

    def __chisq(self,fun, popt, bounds):
        fit = lambda x: fun(x, *popt)
        S = 0
        if self.y_error is not None:
            for i in range(bounds[0],len(self.y_data[:bounds[1]])):
                S += ((self.y_data[i] - fit(self.x_data[i]))/self.y_error[i])**2
        else:
            for i in range(bounds[0],len(self.y_data[:bounds[1]])):
                S += (self.y_data[i] - fit(self.x_data[i]))**2
        return S


    def __output(self, fun, popt, pcov, chisq, dof):

        print("Fit function used: " + fun.__name__ + "\n")
        print("{:20} {:10d}".format("degrees of freedom:", dof))
        print("{:20} {:10.2E}".format("Least square sum:", chisq))
        print("{:20} {:10.2E}".format("reduced chi-square:", chisq/dof))
        print()
        args,_,_,_ = inspect.getargspec(fun)

        print("{:>10} |   ".format("params"), end=" ")
        for arg in args[1:]:
            print(" {:11}".format(arg), end=" ")

        sep = 10*"_" + "|" + 4*"_" + 12*len(popt)*"_"
        print("\n",sep)

        print("{:>12}".format("value |"), end=" ")
        for para in popt:
            print("{:11.3E} ".format(para),end=" ")
        print("")

        sigma = np.sqrt(np.diagonal(pcov))
        print("{:>12}".format("sigma |"), end=" ")
        for sig in sigma:
            print("{:11.3E} ".format(sig),end=" ")
        print("\n")
        return

    def curvefit(self,fun, p0=None, output = False, bounds=(0, None)):
        self.funs.append(fun)
        self.bounds.append(bounds)
        popt, pcov = opt.curve_fit(fun, self.x_data[bounds[0]:bounds[1]],
                                    self.y_data[bounds[0]:bounds[1]], p0 = p0,
                                    sigma = self.y_error[bounds[0]:bounds[1]],
                                    absolute_sigma = True)
        self.popts.append(popt[:])
        self.pcovs.append(pcov[:])

        chisq = self.__chisq(fun, list(popt), bounds)
        dof =  len(self.x_data[bounds[0]:bounds[1]])- len(popt)
      

        if(output):
            self.__output(fun,popt,pcov, chisq, dof)

        return popt, pcov, dof, chisq

    def getPlotData(self, title = "", x_axis = "", y_axis = "", fitres = 200, x_range = None, y_range = None, connected = False, disp_error = True):
        traces = []
        trace1 = go.Scatter(
            x = self.x_data,
            y = self.y_data,
            mode = 'markers' if connected == False else 'lines',
            marker=go.Marker(color='rgb(255, 127, 14)'),
            name = 'Data',
            error_y = dict(
                type='data',
                array = self.y_error,
                visible = True,
                color = 'rgb(180, 0, 0)'
                ) if self.y_error is not None and disp_error is not False else dict()
                )
        traces.append(trace1)

        for i in range(0, len(self.funs)):
            x_fit = np.linspace(self.x_data[self.bounds[i][0]], self.x_data[-1 if self.bounds[i][1] == None else self.bounds[i][1]], fitres)
            y_fit = self.funs[i](np.array(x_fit), *self.popts[i])
            color = np.random.randint(255,size = 3)
            trace = go.Scatter(
                x = x_fit,
                y = y_fit,
                mode = 'line',
                marker=go.Marker(color='rgb(%d, %d, %d)'%(color[0], color[1],color[2])),
                name = self.funs[i].__name__
                )
            traces.append(trace)

        layout = go.Layout(
            title = title,
            xaxis=dict(
                title = x_axis,
                titlefont=dict(
                    family='Courier New, monospace',
                    size=18,
                    color='#7f7f7f',
                    ),
                range = x_range
                ),
            yaxis=dict(
                title = y_axis,
                titlefont=dict(
                    family='Courier New, monospace',
                    size=18,
                    color='#7f7f7f'
                    ),
                range = y_range
                ),
        )

        return go.Figure(data=traces, layout = layout)

class FuncPlot:
    def __init__(self,fun, x_min, x_max):
        self.fun   = fun
        self.x_min  = x_min
        self.x_max  = x_max

    def getPlotData(self, title = "", x_axis = "", y_axis = "", fitres = 200):
            x_fit = np.linspace(self.x_min, self.x_max, fitres)
            trace = go.Scatter(
            x = x_fit,
            y = np.fromiter(map(self.fun,x_fit), dtype = np.float),
            mode = 'line',
            marker=go.Marker(color='rgb(255, 127, 14)'),
            name = "test"
            )
            layout = go.Layout(
                title = title,
                xaxis=dict(
                    title = x_axis,
                    titlefont=dict(
                        family='Courier New, monospace',
                        size=18,
                        color='#7f7f7f'
                        )
                    ),
                yaxis=dict(
                    title = y_axis,
                    titlefont=dict(
                        family='Courier New, monospace',
                        size=18,
                        color='#7f7f7f'
                        )
                    ),
            )
            return go.Figure(data=[trace], layout=layout)
