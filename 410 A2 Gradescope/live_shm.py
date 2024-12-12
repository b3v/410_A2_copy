#!/usr/bin/env python3

import matplotlib.pyplot as plt

# Initialize plot
plt.figure()
plt.xlabel('Time')
plt.ylabel('Temp')
plt.title('Temp vs. Time')
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
    x, y = map(float, data.split())
    # print("live ", x, y )
    if not (x == -1):
        update_plot(x, y)
    else:
        break
    
plt.show()