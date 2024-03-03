'''
Interfacing with fermd services
'''

import flask
import flask_restful
import dateutil.parser
import datetime
import copy
from pprint import pprint

import aegir.db
import aegir.zmq

def init(app, api):
    api.add_resource(Fermds, '/api/fermds')
    pass

class Fermds(flask_restful.Resource):
    def get(self):
        db = aegir.db.Connection()

        return {'status': 'success',
                'data': [x.json for x in db.getFermds()]}

    def post(self):
        '''
        Adds a new fermd instance
        '''
        data = flask.request.get_json();

        for field in ['name', 'address']:
            if not field in data:
                return {'status': 'error',
                        'message': 'field {f} missing'.format(f=field)}, 400
            pass

        # now test the new fermd
        try:
            zmq = aegir.zmq.ZMQReq(data['address'])
            zmq.prmessage('hello')
        except Exception as e:
            return {'status': 'error',
                    'message': 'Unable to contact fermd: {m}'.format(m=str(e))}, 400

        fermd = None
        db = aegir.db.Connection()
        try:
            fermd = db.addFermd(data['name'],
                                data['address'])
        except Exception as e:
            return {'status': 'error',
                    'message': 'Unable to store fermd: {m}'.format(m=str(e))}, 400

        return {'status': 'success',
                'data': fermd.json}
    pass
