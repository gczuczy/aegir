#!/usr/bin/env python3.6

import zmq
import json
from pprint import pprint

ctx = zmq.Context.instance()

sub = ctx.socket(zmq.SUB)
sub.connect("tcp://127.0.0.1:42011")
sub.setsockopt(zmq.SUBSCRIBE, b"")

while True:
    msg = sub.recv()
    pprint(msg)
    pass
