#! /usr/bin/python3

import oculus_python


def message_callback(msg):
    print("Got message :", msg.header())
    print(type(msg.data()))
    # print(data.shape)
    # print(data[0])

c = None
def ping_callback(metadata, data):
    # print("Got message :", metadata)
    # print(metadata.fireMessage.masterMode)
    # print(type(data))
    # print(data.shape)
    # print(data[0])
    global c
    c = metadata.fireMessage

def status_callback(status):
    print("Got status :", status)

def switch_master_mode(sonar):
    c = sonar.current_config()
    print("Current config :")
    print(c)
    if c.masterMode == 1:
        c.masterMode = 2;
    else:
        c.masterMode = 1
    c.pingRate = 0
    print("Config request :")
    print(c)
    print(sonar.send_config(c))

def print_config(sonar):
    print(sonar.current_config())

sonar = oculus_python.OculusSonar()
sonar.start()

sonar.add_message_callback(message_callback)
# sonar.add_ping_callback(ping_callback);
# sonar.add_status_callback(status_callback)

sonar.recorder_start("output.oculus", True)

