#! /usr/bin/python3

import oculus_python

def message_callback(header, data):
    print("Got message :", header)
    print(type(data))
    print(data.shape)
    print(data[0])

def ping_callback(metadata, data):
    print("Got message :", metadata)
    print(type(data))
    print(data.shape)
    print(data[0])

def status_callback(status):
    print("Got status :", status)

sonar = oculus_python.OculusSonar()
sonar.start()

# sonar.add_message_callback(message_callback)
sonar.add_ping_callback(ping_callback);
# sonar.add_status_callback(status_callback)


