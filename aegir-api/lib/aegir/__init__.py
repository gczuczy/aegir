# aegir API library

from flask import Flask
from flask_restful import Api
from pprint import pprint

import aegir.config
import aegir.api
import aegir.zmq
import aegir.db

_app = None
_api = None

def init(cfgfile):
    global _app, _api

    aegir.config.init(cfgfile)

    _app = Flask(__name__)
    _api = Api(_app)

    aegir.zmq.init(_app)
    aegir.db.init(_app)
    aegir.api.init(_app, _api)
    pass

def run():
    _app.run(port=aegir.config.config['port'],
             debug=aegir.config.config['debug'])
    pass
