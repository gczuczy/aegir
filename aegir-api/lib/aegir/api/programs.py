'''
Listing of programs, and defining new ones
'''

import flask
import flask_restful
from pprint import pprint

import aegir.db

def init(app, api):
    api.add_resource(Programs, '/api/programs')
    pass

# {'boiltime': 60,
#    'endtemp': 80,
#    'hops': [{'attime': 60, 'name': '345345', 'quantity': 1},
#                        {'attime': 0, 'name': 'ffgdsfgfg', 'quantity': 123}],
#    'mashsteps': [{'holdtime': 15, 'order': 0, 'temp': 42},
#                                  {'holdtime': 15, 'order': 1, 'temp': 63},
#                                  {'holdtime': 15, 'order': 2, 'temp': 69}],
#    'name': 'asdfadsf',
#    'starttemp': 37}]
def validateProgram(prog):
    errors = []

    # check the name first
    if len(prog['name']) < 3 or len(prog['name']) > 32:
        errors.append('Name must be at least 3, but atmost 32 characters long')
        pass

    # check whether it exists already
    if not aegir.db.checkprogramname(prog['name']):
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

    # validate the hops
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

    if len(errors) == 0:
        return None
    return errors

class Programs(flask_restful.Resource):
    def get(self):
        return {'status': 'success',
                'data': aegir.db.getprograms()}

    def post(self):
        data = flask.request.get_json();
        errors = validateProgram(data)
        if errors:
            return {'status': 'error', 'errors': errors}, 422

        # now add it
        try:
            aegir.db.addprogram(data)
        except KeyError as e:
            return {'status': 'error',
                    'errors': ['KeyError: {name}'.format(name=str(e))]}, 400
        except Exception as e:
            return {'status': 'error',
                    'errors': [str(e)]}, 400

        return {'status': 'success',
                'errors': None}
