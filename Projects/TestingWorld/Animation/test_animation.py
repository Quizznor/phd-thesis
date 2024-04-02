import numpy as np
import matplotlib.pyplot as plt
from matplotlib import animation
import matplotlib
# matplotlib.use('Qt5Agg')

#The bungee rope has to have a [k = 0.15] in order to do 5 oscillations
N = 1
dt = 0.1 #//size step
time = 8000
count =1;
b = 1;
k = 0.2;
m= 100;

# function to open an x-window
fig = plt.figure()

#define positions and velocities for each
x = np.zeros((N,time))
y = np.zeros((N,time))
vx = np.zeros((N,time))
vy = np.zeros((N,time))

#define variables for the plotting canvas
xmin = -5.0; xmax =5.0;
ymin = -80.0; ymax = 80.0;

ax = plt.axes(xlim= (xmin,xmax), ylim=(ymin,ymax),ylabel = 'Y Position')
(my_point,) = ax.plot([],[],'ro',ms=7) #stored var to make red circle (ro) point


#set initial position and velocity
x[:N,0] = 0; y[:N,0] = 75;
vx[:N,0] = 0; vy[:N,0] = 0.0;

for i in range(time-1):
    #set path/function
    for j in range(N):
        a = (-k*y[j,i] - b*vy[j,i])/(m); #damped simple harmonic motion
        vy[j,i+1] = vy[j,i] +  a*dt #differential changes to velocity
        y[j,i+1] = y[j,i] + dt*vy[j,i+1] #differential changes to distance

def get_step(n,x,y,point):
    point.set_data(x[n],y[n])
    return point,

mymovie = animation.FuncAnimation(fig,get_step,frames= time, blit=True,
                                  fargs=(x[0,:],y[0,:],my_point), interval = 0)   

plt.show()
print("For 5 oscillations k = %f\n" % k)