'''
ZMQ handling, takes care of the per-flask-instance initialization, and
provides a highlevel communication API for the app
'''

import zmq
from pprint import pprint

import aegir.config

_ctx = None
_socket_pr = None

def init(app):
    app.before_first_request(instance_init)
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
    _socket_pr.connect('tcp://127.0.0.1:{port}'.format(port = aegir.config.config['prport']))
    _socket_pr.setsockopt(zmq.RCVTIMEO, 1000)
    _socket_pr.setsockopt(zmq.LINGER, 0)

def prmessage(command, data):
    global _socket_pr

    msg = {'command': command,
           'data': data}
    try:
        _socket_pr.send_json(msg)
        return _socket_pr.recv_json()
    except Exception:
        reconnect_socket()
        raise Exception("brewd is not running")
    pass
