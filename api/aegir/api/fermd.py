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
    api.add_resource(Fermenter, '/api/fermds/<int:fermdid>/fermenters/<int:fid>')
    api.add_resource(SensorCache, '/api/fermds/<int:fermdid>/sensorcache')
    api.add_resource(Yeasts, '/api/fermds/<int:fermdid>/yeasts')
    api.add_resource(Yeast, '/api/fermds/<int:fermdid>/yeasts/<int:yeastid>')
    api.add_resource(Brews, '/api/fermds/<int:fermdid>/brews')
    api.add_resource(Brew, '/api/fermds/<int:fermdid>/brew/<int:brewid>')
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

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('getTilthydrometers')
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400
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

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('getFermenterTypes')
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400

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

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('addFermenterTypes', data)
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400
    pass

class FermenterType(flask_restful.Resource):
    def delete(self, fermdid, ftid):
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('deleteFermenterTypes', {'id': ftid})
            return {'status': 'success'}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400

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

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('updateFermenterTypes', {'id': ftid,
                                                          'name': data['name'],
                                                          'capacity': int(data['capacity']),
                                                          'imageurl': data['imageurl']})
            return {'status': 'success'}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400

    pass

class Fermenters(flask_restful.Resource):
    def get(self, fermdid):
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('getFermenters')
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400

    def post(self, fermdid):
        '''
        Adds a new fermenter
        '''
        data = flask.request.get_json();

        for field in ['name', 'type']:
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

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('addFermenter', data)
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400
        pass
    pass

class SensorCache(flask_restful.Resource):
    def get(self, fermdid):
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('getSensorCache')
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400

class Fermenter(flask_restful.Resource):
    def post(self, fermdid, fid):
        '''
        Updates a fermenter
        '''
        data = flask.request.get_json();
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        data['id'] = fid
        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('updateFermenter', data)
        except Exception as e:
            return {'status': 'error',
                    'message': str(e)},400
        return {'status': 'success'}

    def delete(self, fermdid, fid):
        '''
        Updates a fermenter
        '''
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        data = {'id': fid}
        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('deleteFermenter', data)
        except Exception as e:
            return {'status': 'error',
                    'message': str(e)},400
        return {'status': 'success'}
    pass

class Yeasts(flask_restful.Resource):
    def get(self, fermdid):
        '''
        Returns all yeasts
        '''
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('getYeasts')
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400
        pass

    def post(self, fermdid):
        '''
        Adds a new yeast
        '''
        data = flask.request.get_json();

        for field in ['name', 'abv', 'attenuation', 'mintemp', 'maxtemp']:
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

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('addYeast', data)
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400
        pass
    pass

class Yeast(flask_restful.Resource):
    def post(self, fermdid, yeastid):
        '''
        Updates a yeast
        '''
        data = flask.request.get_json();
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        data['id'] = yeastid
        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('updateYeast', data)
        except Exception as e:
            return {'status': 'error',
                    'message': str(e)},400
        return {'status': 'success',
                'data': resp['data']}

    def delete(self, fermdid, yeastid):
        '''
        Updates a yeast
        '''
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        data = {'id': yeastid}
        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('deleteYeast', data)
        except Exception as e:
            return {'status': 'error',
                    'message': str(e)},400
        return {'status': 'success'}

class Brews(flask_restful.Resource):
    def get(self, fermdid):
        '''
        Returns all brews
        '''
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('getBrews')
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400
        pass

    def post(self, fermdid):
        '''
        Adds a new brew
        '''
        data = flask.request.get_json();

        for field in ['name', 'yeast']:
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

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('addBrew', data)
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400
        pass
    pass

class Brew(flask_restful.Resource):
    def get(self, fermdid, brewid):
        '''
        Returns a brew
        '''
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('getBrew', {'id': brewid})
            return {'status': 'success',
                    'data': resp['data']}
        except Exception as e:
            return {'status': 'error',
                    'message': 'ZMQ error: {e}'.format(e=str(e))},400
        pass

    def post(self, fermdid, brewid):
        '''
        Updates a brew
        '''
        data = flask.request.get_json();
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        data['id'] = yeastid
        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('updateBrew', data)
        except Exception as e:
            return {'status': 'error',
                    'message': str(e)},400
        return {'status': 'success',
                'data': resp['data']}

    def delete(self, fermdid, brewid):
        '''
        Updates a brew
        '''
        db = aegir.db.Connection()
        try:
            fermd = db.getFermd(fermdid)
        except Exception as e:
            return {'status': 'error',
                    'message': 'No such fermd: {e}'.format(e=str(e))},400

        data = {'id': yeastid}
        try:
            zmq = aegir.zmq.ZMQReq(fermd.address)
            resp = zmq.prmessage('deleteBrew', data)
        except Exception as e:
            return {'status': 'error',
                    'message': str(e)},400
        return {'status': 'success'}

