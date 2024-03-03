'''
ZMQ handling, takes care of the per-flask-instance initialization, and
provides a highlevel communication API for the app
'''

import zmq
import json
from pprint import pprint

_ctx = None

def init(app):
    pass

class Singleton(type):
    _instances = {}
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(singleton, cls).__call__(*args, **kwargs)
            pass
        return cls._instances[cls]
        pass
    pass

class ZMQReq:
    __metaclass__ = Singleton
    def __init__(self, addr):
        global _ctx
        if _ctx is None: _ctx = zmq.Context()

        self._addr = addr
        self._reconnect()
        pass

    def _reconnect(self):
        self._socket = _ctx.socket(zmq.REQ)
        self._socket.connect(self._addr)
        self._socket.setsockopt(zmq.RCVTIMEO, 1000)
        self._socket.setsockopt(zmq.LINGER, 0)
        pass

    def prmessage(self, command, data=None):
        msg = {'command': command,
               'data': data}
        result = {}
        try:
            self._socket.send_json(msg)
        except Exception as e:
            raise Exception("Error while sending message: {msg}"
                            .format(msg = str(e)))
        try:
            reslt = self._socket.recv_json()
        except Exception as e:
            #pprint(['zmq_recv', e])
            raise Exception("Cannot parse response: {err}"
                            .format(err = str(e)))

        if 'status' in result and result['status'] == 'error':
            if 'message' in result:
                raise Exception('{cmd} error: {msg}'.format(cmd = command,
                                                            msg = resuilt['message']))
            else:
                raise Exception('Unknown {cmd} error'.format(cmd = command))
            pass
        return result

    pass

