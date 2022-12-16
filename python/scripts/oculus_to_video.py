#! /usr/bin/python3

import os
import numpy as np
import matplotlib.pyplot as plt
import argparse

from oculus_python.files import OculusFileReader
from oculus_python.utils import VideoEncoder

parser = argparse.ArgumentParser(
    prog='OculusFileReader',
    description='Example of how to read and display the content of a .oculus ' +
                'file. This will display the first ping from a the file.')
parser.add_argument('filename', type=str,
                    help='Path to a .oculus file to display')
parser.add_argument('-o', '--output', type=str, default=None,
                    help='output video filename (default is same name as input with a .mp4 extension)')
parser.add_argument('-f', '--overwrite', action='store_true',
                    help='Overwrite existing file.')
args = parser.parse_args()

if args.output is None:
    args.output = os.path.splitext(os.path.split(args.filename)[1])[0] + '.mp4'

print('Opening', args.filename)

f = OculusFileReader(args.filename)

msg = f.read_next_ping() # this can be called several time to iterate through the pings
                         # will return None when finished
if msg is None:
    raise ValueError('File seems to be empty.')

# first getting the ping rate
count = 0
t0 = msg.timestamp()
print("First ping date :", t0)

t  = t0
for i in range(10):
    msg = f.read_next_ping()
    if msg is None:
        break
    count += 1
    t = msg.timestamp()

if count < 2:
    raise ValueError('Only one ping in .oculus. Cannot make video.')

pingRate = (count - 1) / (t - t0).total_seconds()
f.rewind()
msg = f.read_next_ping()

encoder = VideoEncoder(videoPath=args.output, frameRate=pingRate)

while msg is not None:
    encoder.add_ping(msg)
    msg = f.read_next_ping()
encoder.finish()




