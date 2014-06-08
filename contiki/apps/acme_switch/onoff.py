#!/usr/bin/env python3

import socket

rfid_node_addr1 = '2001:470:1f11:131a:c298:e541:4310:2'
#rfid_node_addr1 = '2001:470:1f11:131a:1034:5678:90ab:cdef'
#rfid_node_addr1 = '2001:470:1f11:131a:60f:7dc6:12:4b00'

s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)

dest = socket.getaddrinfo(rfid_node_addr1, None)[0][-1]
print(dest)
print(b'1')
s.sendto('\x01'.encode(), (rfid_node_addr1, 47652))
s.close()

quit()

rfid_node_addr2 = '2001:470:1f11:131a:60f:7dc6:12:4b00'

s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)

dest = socket.getaddrinfo(rfid_node_addr2, None)[0][-1]
print(dest)
print(b'1')
s.sendto('\x02'.encode(), (rfid_node_addr2, 47652))
s.close()

