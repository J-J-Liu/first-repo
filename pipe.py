import os
import struct

# 定义结构体格式：两个int和一个100字符的字符串
struct_format = 'ii100s'

fifo_path = '/tmp/my_fifo'

# 打开命名管道
with open(fifo_path, 'rb') as fifo:
    while True:
        # 读取结构体数据
        data = fifo.read(struct.calcsize(struct_format))
        if data:
            # 解包数据
            unpacked_data = struct.unpack(struct_format, data)
            a, b, raw_str = unpacked_data
            str_decoded = raw_str.split(b'\x00', 1)[0].decode('utf-8')  # 去掉字符串中的空字符
            print(f'Read: {a} {b} {str_decoded}')
        else:
            break
