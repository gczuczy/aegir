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
    api.add_resource(TiltHydrometers, '/api/fermds/<int:fermdid>/tilthydrometers')
    api.add_resource(TiltHydrometer, '/api/fermds/<int:fermdid>/tilthydrometers/<int:tiltid>')
    api.add_resource(FermenterTypes, '/api/fermds/<int:fermdid>/fermentertypes')
    api.add_resource(FermenterType, '/api/fermds/<int:fermdid>/fermentertypes/<int:ftid>')
    api.add_resource(Fermenters, '/api/fermds/<int:fermdid>/fermenters')
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
        try:
            fermd = db.addFermd(data['name'],
                                data['address'])
        except Exception as e:
            return {'status': 'error',
                    'message': 'Unable to store fermd: {m}'.format(m=str(e))}, 400

        return {'status': 'success',
                'data': fermd.json}
    pass

class TiltHydrometers(flask_restful.Resource):
    def get(self, fermdid):
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        zmq = aegir.zmq.ZMQReq(fermd.address)
        resp = zmq.prmessage('getTilthydrometers')
        return {'status': 'success',
                'data': resp['data']}
    pass

class TiltHydrometer(flask_restful.Resource):
    def post(self, fermdid, tiltid):
        data = flask.request.get_json();
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        data['id'] = tiltid
        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('updateTilthydrometer', data)
        except Exception as e:
            return {'status': 'error',
                    'message': str(e)},400
        return {'status': 'success',
                'data': resp['data']}
    pass

class FermenterTypes(flask_restful.Resource):
    def get(self, fermdid):
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        zmq = aegir.zmq.ZMQReq(fermd.address)
        resp = zmq.prmessage('getFermenterTypes')
        return {'status': 'success',
                'data': resp['data']}

    def post(self, fermdid):
        '''
        Adds a new fermenter type
        '''
        data = flask.request.get_json();

        for field in ['name', 'capacity', 'imageurl']:
            if not field in data:
                return {'status': 'error',
                        'message': 'field {f} missing'.format(f=field)}, 400
            pass

        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        zmq = aegir.zmq.ZMQReq(fermd.address)
        resp = zmq.prmessage('addFermenterTypes', data)
        return {'status': 'success',
                'data': resp['data']}
    pass

class FermenterType(flask_restful.Resource):
    def delete(self, fermdid, ftid):
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        zmq = aegir.zmq.ZMQReq(fermd.address)
        resp = zmq.prmessage('deleteFermenterTypes', {'id': ftid})
        return {'status': 'success'}

    def post(self, fermdid, ftid):
        '''
        Updates a fermenter type
        '''
        data = flask.request.get_json();

        for field in ['name', 'capacity', 'imageurl']:
            if not field in data:
                return {'status': 'error',
                        'message': 'field {f} missing'.format(f=field)}, 400
            pass

        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        zmq = aegir.zmq.ZMQReq(fermd.address)
        resp = zmq.prmessage('updateFermenterTypes', {'id': ftid,
                                                      'name': data['name'],
                                                      'capacity': int(data['capacity']),
                                                      'imageurl': data['imageurl']})
        return {'status': 'success'}
    pass

class Fermenters(flask_restful.Resource):
    def get(self, fermdid):
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        zmq = aegir.zmq.ZMQReq(fermd.address)
        resp = zmq.prmessage('getFermenters')
        return {'status': 'success',
                'data': resp['data']}

    pass
