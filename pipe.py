import os
import struct
import time

# 定义结构体格式：两个int和一个100字符的字符串
struct_format = 'ii100s'
struct_size = struct.calcsize(struct_format)

fifo_path = '/tmp/my_fifo'

# 打开命名管道
with open(fifo_path, 'rb') as fifo:
    while True:
        data = fifo.read(struct_size)
        if data:
            # 解包数据
            unpacked_data = struct.unpack(struct_format, data)
            a, b, raw_str = unpacked_data
            str_decoded = raw_str.split(b'\x00', 1)[0].decode('utf-8')  # 去掉字符串中的空字符
            print(f'Read: {a} {b} {str_decoded}')
        else:
            # 检查管道是否已经没有更多的数据，如果没有更多的数据，就等待一段时间再检查
            time.sleep(1)
            remaining_data = fifo.read(struct_size)
            if not remaining_data:
                break
            else:
                # 将剩余数据打印
                unpacked_data = struct.unpack(struct_format, remaining_data)
                a, b, raw_str = unpacked_data
                str_decoded = raw_str.split(b'\x00', 1)[0].decode('utf-8')
                print(f'Read: {a} {b} {str_decoded}')
