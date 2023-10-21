'''
Listing of programs, and defining new ones
'''

import flask
import flask_restful
from pprint import pprint

import aegir.db

def init(app, api):
    api.add_resource(Programs, '/api/programs')
    api.add_resource(Program, '/api/programs/<int:progid>')
    pass

def validateProgram(prog):
    errors = []
    dbc = aegir.db.Connection()

    # check the name first
    if len(prog['name']) < 3 or len(prog['name']) > 32:
        errors.append('Name must be at least 3, but atmost 32 characters long')
        pass

    # check whether it exists already
    progid = None
    if 'id' in prog:
        progid = prog['id']
    if not dbc.checkProgramName(prog['name'], progid):
        errors.append('A program with the same name already exists')
        pass

    # check the boil time
    if prog['boiltime'] < 0 or prog['boiltime'] > 300:
        errors.append('Boiltime must be at least 0 and bellow 300 minutes')
        pass

    # check the start/end temp
    if not prog['starttemp'] < prog['endtemp']:
        errors.append('Start temperature must be smaller than end temperature')
        pass

    # check the mash steps
    if not prog['nomash']:
        for i in range(len(prog['mashsteps'])):
            mintemp = prog['starttemp']
            if i != 0:
                mintemp = prog['mashsteps'][i-1]['temp']
                pass
            maxtemp = prog['endtemp']
            if i != len(prog['mashsteps'])-1:
                maxtemp = prog['mashsteps'][i+1]['temp']
                pass
            if prog['mashsteps'][i]['temp'] < mintemp or prog['mashsteps'][i]['temp'] > maxtemp:
                errors.append("Mash steps temperatures are out of line {temp}".format(temp=prog['mashsteps'][i]['temp']))
                pass
            if not prog['mashsteps'][i]['order'] == i:
                errors.append('Mash Step order out of place: {order}!={i}'.format(order=prog['mashsteps'][i]['order'],
                                                                                  i=i))
                pass
            pass
        pass

    # validate the hops
    if not prog['noboil']:
        for hop in prog['hops']:
            if hop['attime'] < 0 or hop['attime'] > prog['boiltime']:
                errors.append('Hop timing must be greater than 0 and less than boiltime')
                pass
            if hop['quantity'] < 0:
                errors.append('Hop quantity must be positive')
                pass
            if len(hop['name']) < 3 or len(hop['name'])>32:
                errors.append('Hop names must be between 3 and 32 characters')
                pass
            pass
        pass

    if len(errors) == 0:
        return None
    return errors

class Programs(flask_restful.Resource):
    def get(self):
        dbc = aegir.db.Connection()
        progs = dbc.getPrograms()
        return {'status': 'success',
                'data': [p.data for p in progs]}

    def post(self):
        '''
        Adds a program. Input is a json data.
        '''
        data = flask.request.get_json();

        # force-sanitize some input
        if data['nomash']:
            data['mashsteps'] = None
            pass
        if data['noboil']:
            data['hops'] = None
            pass

        errors = validateProgram(data)
        if errors:
            return {'status': 'error', 'errors': errors}, 422

        # now add it
        progid = None
        try:
            dbc = aegir.db.Connection()
            progid = dbc.addProgram(data)
        except KeyError as e:
            return {'status': 'error',
                    'errors': ['KeyError: {name}'.format(name=str(e))]}, 400
        except Exception as e:
            return {'status': 'error',
                    'errors': [str(e)]}, 400

        return {'status': 'success',
                'data': {'progid': progid}}
    pass

class Program(flask_restful.Resource):
    '''
    Deals with a single program
    '''
    def get(self, progid):
        pprint(['Program', progid])
        res = None
        try:
            dbc = aegir.db.Connection()
            res = dbc.getProgram(progid)
        except Exception as e:
            return {'status': 'error',
                    'errors': [str(e)]}, 400

        return {'status': 'success',
                'data': res.data}

    def delete(self, progid):
        try:
            aegir.db.Connection().getProgram(progid).remove()
        except Exception as e:
            return {'status': 'error',
                    'errors': [str(e)]}, 400
        return {'status': 'success'}

    def post(self, progid):
        data = flask.request.get_json();
        #pprint(data)
        errors = validateProgram(data)
        if errors:
            return {'status': 'error', 'errors': errors}, 422

        # now save it
        try:
            aegir.db.Connection().getProgram(progid).update(data)
        except KeyError as e:
            return {'status': 'error',
                    'errors': ['KeyError: {name}'.format(name=str(e))]}, 400
        except Exception as e:
            return {'status': 'error',
                    'errors': [str(e)]}, 400

        return {'status': 'success',
                'data': {'progid': progid}}
pass
