#! /usr/bin/python3

import time
import argparse
import oculus_python

parser = argparse.ArgumentParser(
    prog='OculusRecord',
    description='Example of how to record data from a connected Oculus sonar')
parser.add_argument('-o', '--output', type=str, default='output.oculus',
                    help='Path where to save the output file.')
parser.add_argument('-r', '--range', type=int, default=10,
                    help='Sonar range in meters')
args = parser.parse_args()

configSet = False
def message_callback(msg):
    global sonar
    global configSet
    global args
    if configSet:
        return

    currentConfig = sonar.current_config()
    currentConfig.range = args.range
    print(currentConfig)
    sonar.send_config(currentConfig)

    print('Start recording to :', args.output)
    sonar.recorder_start(args.output, True)
    configSet = True
    
sonar = oculus_python.OculusSonar()
sonar.start()

sonar.add_message_callback(message_callback)

def message_callback2(msg):
    print("Got message")
    print(msg.header())
sonar.add_message_callback(message_callback2)


time.sleep(2)
input("Press a key to quit")

sonar.stop()
sonar.recorder_stop()



