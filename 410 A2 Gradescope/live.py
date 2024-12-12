#!/usr/bin/env python3
import matplotlib 
matplotlib.use('Agg')
import matplotlib.pyplot as plt


# Initialize plot
plt.figure()
plt.xlabel('Sample Number')
plt.ylabel('Arg')
plt.title('X vs Y')
plt.grid(True)

# Start live plotting
x_values = []
y_values = []

def update_plot(x, y):
   
    x_values.append(x)
    y_values.append(y)
    plt.plot(x_values, y_values, 'b-+')
    plt.pause(0.01)

while True:
    
    data = input("")
    splat = data.split()
    x_split = splat[0].split("=")

    x_name = x_split[0]
    x = float(x_split[1])
    y = float(splat[1])
    print("live ", int(x), y )
    if not (int(x) == -1):

        update_plot(x, y)
    else:
        print("we break")
        
        break
    
plt.show()