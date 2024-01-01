'''
ZMQ handling, takes care of the per-flask-instance initialization, and
provides a highlevel communication API for the app
'''

import zmq
import json
from pprint import pprint

import aegir.config

_ctx = None
_socket_pr = None

def init(app):
    with app.app_context():
        instance_init()
        pass
    pass

def instance_init():
    global _ctx, _socket_pr

    _ctx = zmq.Context()
    reconnect_socket();
    pass

def reconnect_socket():
    global _ctx, _socket_pr

    if not _socket_pr == None:
        _socket_pr.close()
        _socket_pr = None
        pass

    _socket_pr = _ctx.socket(zmq.REQ)
    addr = 'tcp://127.0.0.1:{port}'.format(port = aegir.config.config['prport'])
    _socket_pr.connect(addr)
    #pprint(['connecting to zmq', addr])
    _socket_pr.setsockopt(zmq.RCVTIMEO, 1000)
    _socket_pr.setsockopt(zmq.LINGER, 0)
    #_socket_pr.setsockopt(zmq.PROBE_ROUTER, 1)

def prmessage(command, data):
    global _socket_pr

    msg = {'command': command,
           'data': data}
    #pprint(msg)
    try:
        _socket_pr.send_json(msg)
    except Exception as e:
        #pprint(['zmq_send', e])
        reconnect_socket()
        raise Exception("Error while sending message: {msg}".format(msg = str(e)))

    try:
        return _socket_pr.recv_json()
    except Exception as e:
        #pprint(['zmq_recv', e])
        raise Exception("Cannot parse brewd response: {err}".format(err = str(e)))
    pass
