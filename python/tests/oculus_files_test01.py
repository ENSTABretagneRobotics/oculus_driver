#! /usr/local/python3

import numpy as np
import matplotlib.pyplot as plt

from oculus_python.files import OculusFileReader

f = OculusFileReader("output.oculus")

msg = f.read_next_ping()

bearings     = 0.01*np.array(msg.bearing_data())
linearAngles = np.linspace(bearings[0], bearings[-1], len(bearings))
rawPingData = np.array(msg.raw_ping_data())
gains = np.ones([msg.range_count(),], dtype=np.float32)
if msg.has_gains():
    gains = np.array(msg.gains())
pingData = np.array(msg.ping_data()) / np.sqrt(gains)[:,np.newaxis]

print('has gains   :', msg.has_gains())
print('sample size :', msg.sample_size())
print('ping shape  :', pingData.shape)

_, ax = plt.subplots(1,1)
ax.plot(bearings,     '-o', label='bearings')
ax.plot(linearAngles, '-o', label='linear bearings')
ax.grid()
ax.legend()
ax.set_xlabel('bearing index')
ax.set_ylabel('bearing angle')

_, ax = plt.subplots(1,1)
ax.plot(gains, '-o', label='gains')
ax.grid()
ax.legend()
ax.set_xlabel('range index')
ax.set_ylabel('range gain')

_, ax = plt.subplots(1,2)
ax[0].imshow(rawPingData)
ax[0].set_ylabel('Range index')
ax[0].set_xlabel('Bearing index')
ax[0].set_title('Raw ping data')

ax[1].imshow(pingData)
ax[1].set_xlabel('Bearing index')
ax[1].set_title('Ping data rescaled with gains')

plt.show(block=False)


