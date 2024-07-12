# myscript.py
import os
import struct

PIPE_NAME = '/tmp/my_pipe'
STRUCT_FORMAT = 'ii50s'

def read_struct_from_pipe():
    with open(PIPE_NAME, 'rb+', buffering=0) as pipe:
        data = pipe.read(struct.calcsize(STRUCT_FORMAT))
        return struct.unpack(STRUCT_FORMAT, data)

# Ensure the pipe exists
if not os.path.exists(PIPE_NAME):
    os.mkfifo(PIPE_NAME)

print("Waiting for data...")
while True:
    struct_data = read_struct_from_pipe()
    id, value, name = struct_data
    name = name.split(b'\0', 1)[0].decode('utf-8')  # Remove null characters and decode

    print(f"id: {id}")
    print(f"value: {value}")
    print(f"name: {name}")
