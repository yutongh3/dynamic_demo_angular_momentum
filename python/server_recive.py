# pyinstaller --onefile --windowed server_recive.py
import socket
import threading
import time
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button
from collections import deque
import yaml

# Load configuration from YAML file
with open('server_config.yaml', 'r') as file:
# with open('server_config_hotspot.yaml', 'r') as file:
    config = yaml.safe_load(file)

recv_ip = config['receive']['ip']
recv_port = config['receive']['port']
send_ip = config['send']['ip']
send_port = config['send']['port']

# Socket setup for data receiving
recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock.bind((recv_ip, recv_port))

# Socket setup for sending commands
send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Data storage
data_queue = deque(maxlen=350)  # Store the last 350 data points
battery_voltage = "Battery: N/A"  # Initial battery voltage message
text_color = 'black'

def receive_data():
    while True:
        data, addr = recv_sock.recvfrom(1024)
        message = data.decode()
        if 'SERVO DUTY' in message:
            servo_duty = message.split(':')
            if len(servo_duty) == 2:
                duty = float(servo_duty[1])
                lock.acquire()
                data_queue.append(duty)
                lock.release()
        elif 'BATTERY V' in message:
            battery_volts = message.split(':')
            if len(battery_volts) == 2:
                voltage = float(battery_volts[1])/1000
                lock.acquire()
                global battery_voltage
                global text_color
                if voltage < 3.8:
                    text_color = 'red'
                else:
                    text_color = 'black'
                battery_voltage = f"Battery: {voltage}V"
                lock.release()

def contract_servo(event):
    message = "CONTRACT"
    send_sock.sendto(message.encode(), (send_ip, send_port))

def expand_servo(event):
    message = "EXPAND"
    send_sock.sendto(message.encode(), (send_ip, send_port))

# Thread for receiving data
lock = threading.Lock()
thread = threading.Thread(target=receive_data)
thread.daemon = True
thread.start()

# Matplotlib setup
fig, ax = plt.subplots()
plt.subplots_adjust(bottom=0.2)  # Make room for the buttons
line, = ax.plot([], [], lw=2)
battery_text = ax.text(0.01, 0.95, battery_voltage, transform=ax.transAxes, verticalalignment='top')

def init():
    ax.set_xlim(0, 350)
    ax.set_ylim(-2500, 2500)
    return line, battery_text

def update(frame):
    lock.acquire()
    ydata = list(data_queue)
    updated_text = battery_voltage
    updated_color = text_color
    lock.release()
    xdata = range(len(ydata))
    line.set_data(xdata, ydata)
    battery_text.set_text(updated_text)
    battery_text.set_color(updated_color)
    return line, battery_text

# Animation
ani = FuncAnimation(fig, update, init_func=init, frames=200, interval=30, blit=True)

# Buttons
ax_contract = plt.axes([0.59, 0.05, 0.15, 0.075])
btn_contract = Button(ax_contract, 'Contract')
btn_contract.on_clicked(contract_servo)

ax_expand = plt.axes([0.75, 0.05, 0.15, 0.075])
btn_expand = Button(ax_expand, 'Expand')
btn_expand.on_clicked(expand_servo)

# Hide the toolbar and rename the window
plt.rcParams['toolbar'] = 'None'
fig.canvas.manager.set_window_title('Servo')

plt.show()
