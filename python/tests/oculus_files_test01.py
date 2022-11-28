#! /usr/local/python3

from oculus_python.files import OculusFileReader

f = OculusFileReader("output.oculus")
msg = f.read_next_message()

while msg is not None:
    print(msg)
    msg = f.read_next_message()



