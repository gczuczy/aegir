'''
This endpoint is responsible for communicating with the brewd backend over ZMQ
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
    api.add_resource(BrewProgram, '/api/brewd/program')
    api.add_resource(BrewConfig, '/api/brewd/config')
    api.add_resource(BrewState, '/api/brewd/state')
    api.add_resource(BrewStateVolume, '/api/brewd/state/volume')
    api.add_resource(BrewStateTempHistory, '/api/brewd/state/temphistory')
    api.add_resource(BrewMaintenance, '/api/brewd/maintenance')
    api.add_resource(BrewOverride, '/api/brewd/override')
    api.add_resource(BrewStateCoolTemp, '/api/brewd/state/cooltemp')
    pass

class BrewProgram(flask_restful.Resource):
    def post(self):
        data = flask.request.get_json();

        program = None
        try:
            program = aegir.db.Connection().getProgram(data['id'])
        except Exception as e:
            return {"status": "error",
                    "errors": [str(e)]}, 422

        progdata = copy.deepcopy(program.data)
        for hop in progdata['hops']:
            hop['attime'] *= 60
            pass

        zreq = {"program": progdata}
        try:
            for field in ['startat', 'startmode', 'volume']:
                if not field in data:
                    raise Exception('Field {f} is missing'.format(f = field))

            vol = int(data['volume'])
            if vol < 5 or vol > 80:
                raise Exception("Volume is out of bounds")
            zreq['volume'] = vol

            if not data['startmode'] in ['now', 'timed']:
                raise Exception('Unknown value for startmode: {sm}'.format(sm = data['startmode']))
        except Exception as e:
            return {"status": "error",
                    "errors": [str(e)]}, 422

        # check the timing
        if data['startmode'] == 'now':
            zreq['startat'] = 0
        elif data['startmode'] == 'timed':
            startat = dateutil.parser.parse(data['startat'])
            now = datetime.datetime.utcnow()
            minbefore = now - datetime.timedelta(minutes=10)
            maxahead = now+datetime.timedelta(days=7)
            if startat < minbefore:
                return {"status": "error",
                        "errors": 'Start datetiem is too early'}, 422
            if startat > maxahead:
                return {"status": "error",
                        "errors": 'You should not plan so much ahead'}, 422
            if startat < now:
                zreq['startat'] = 0
            else:
                zreq['startat'] = int(startat.timestamp())
        else:
            return {"status": "error",
                    "errors": "Unknown start mode: {sm}".format(sm = zreq['startmode'])}, 422

        pprint(['loading', zreq])
        zresp = None
        try:
            zresp = aegir.zmq.prmessage("loadProgram", zreq)
        except Exception as e:
            return {"status": "error",
                    "errors": [str(e)]}, 422
        #pprint(['zresp', zresp])

        if zresp['status'] != 'success':
            return zresp, 422

        return zresp
    pass

class BrewConfig(flask_restful.Resource):
    def get(self):
        zresp = None
        try:
            zresp = aegir.zmq.prmessage("getConfig", None)
        except Exception as e:
            pprint(e)
            return {"status": "error", "errors": [str(e)]}, 422

        pprint(zresp)
        if not 'status' in zresp:
            return {"status": "error", "errors": ['Malformed response']}, 422

        if zresp['status'] != 'success':
            return {"status": "error", "errors": ['Error in response']}, 422

        return {'status': 'success', 'data': zresp['data']}

    def post(self):
        '''
        Set configuration variables
        hepower, tempaccuracy, heatoverhead, cooltemp
        '''
        data = flask.request.get_json();
        zresp = None
        zdata = {}

        for var in ['hepower', 'tempaccuracy', 'heatoverhead', 'cooltemp']:
            if var in data:
                zdata = data[var]
                pass
            pass

        try:
            zresp = aegir.zmq.prmessage('setConfig', zdata)
        except Exception as e:
            #pprint(['error', zresp, e]);
            return {"status": "error", "errors": [str(e)]}, 422

        if not 'status' in zresp:
            return {"status": "error", "errors": ['Malformed response']}, 422

        if zresp['status'] != 'success':
            #pprint(['error in zresp', zresp])
            return {"status": "error", "errors": ['Error in response']}, 422

        return {'status': 'success'}

class BrewState(flask_restful.Resource):
    def get(self):
        history = flask.request.args.get('history', None)
        # handle the defaults
        needhistory = history == 'yes'
        #pprint(['got getstate request'])

        zresp = None
        try:
            zresp = aegir.zmq.prmessage("getState", {"history": needhistory})
        except Exception as e:
            #pprint(e)
            return {"status": "error", "errors": [str(e)]}, 422

        #pprint(zresp)
        if not 'status' in zresp:
            return {"status": "error", "errors": ['Malformed response']}, 422

        if zresp['status'] != 'success':
            return {"status": "error", "errors": ['Error in response']}, 422

        return {'status': 'success', 'data': zresp['data']}

    def post(self):
        '''
        POST is used to indicate state controls, such as having malts added,
        whether sparging is completed and boiling can begin, etc.
        '''
        data = flask.request.get_json();
        zresp = None

        if not 'command' in data:
            return {"status": "error", "errors": ['Missing command member in data']}, 422

        zcmd = None
        zdata = None
        if data['command'] == 'hasMalt':
            zcmd = 'hasMalt'
        elif data['command'] == 'spargeDone':
            zcmd = 'spargeDone'
        elif data['command'] == 'startBoil':
            zcmd = 'startHopping'
        elif data['command'] == 'coolingDone':
            zcmd = 'coolingDone'
        elif data['command'] == 'transferDone':
            zcmd = 'transferDone'
        elif data['command'] == 'reset':
            zcmd = 'resetProcess'
        else:
            return {"status": "error", "errors": ['Unknown command']}, 422

        try:
            zresp = aegir.zmq.prmessage(zcmd, zdata)
        except Exception as e:
            #pprint(['error', zresp, e]);
            return {"status": "error", "errors": [str(e)]}, 422

        if not 'status' in zresp:
            return {"status": "error", "errors": ['Malformed response']}, 422

        if zresp['status'] != 'success':
            #pprint(['error in zresp', zresp])
            return {"status": "error", "errors": ['Error in response']}, 422

        return {'status': 'success'}

class BrewStateVolume(flask_restful.Resource):
    '''
    This class allows the manipulation of the current Process'
    volume
    '''
    def get(self):
        '''
        Retrieves and returns the current volume set
        '''

        zcmd = 'getVolume'
        zresp = None
        try:
            zresp = aegir.zmq.prmessage(zcmd, None)
        except Exception as e:
            #pprint(['error', zresp, e]);
            return {"status": "error", "errors": [str(e)]}, 422

        if 'status' not in zresp:
            return {"status": "error", "errors": ['Malformed zmq message']}, 422

        if zresp['status'] == 'error':
            return {"status": "error", "errors": [zresp['message']]}, 422

        return {'status': 'success', 'data': zresp['data']}

    def post(self):
        '''
        Sets a new volume
        '''
        data = flask.request.get_json();
        zcmd = 'setVolume';
        zdata = {'volume': data['volume']}
        try:
            zresp = aegir.zmq.prmessage(zcmd, zdata)
        except Exception as e:
            return {"status": "error", "errors": [str(e)]}, 422

        return {'status': 'success'}

class BrewStateTempHistory(flask_restful.Resource):
    '''
    Fetches the temperature history
    '''
    def get(self):
        '''
        Retrieves and returns the current volume set
        '''

        zcmd = 'getTempHistory'
        frm = flask.request.args.get('from', None)
        data = {}
        if not frm is None:
            try:
                data['from'] = int(frm)
            except Exception as e:
                return {"status": "error", "errors": [str(e)]}, 422
            pass

        zresp = None
        try:
            zresp = aegir.zmq.prmessage(zcmd, data)
        except Exception as e:
            #pprint(['error', zresp, e]);
            return {"status": "error", "errors": [str(e)]}, 422

        if 'status' not in zresp:
            return {"status": "error", "errors": ['Malformed zmq message']}, 422

        if zresp['status'] == 'error':
            return {"status": "error", "errors": [zresp['message']]}, 422

        return {'status': 'success', 'data': zresp['data']}

    pass

class BrewMaintenance(flask_restful.Resource):
    '''
    Maintenance-mode related calls
    '''
    def put(self):
        '''
        Putting into maintmode, and finishing it
        '''
        data = flask.request.get_json();

        if not isinstance(data, dict) or not 'mode' in data:
            return {"status": "error", "errors": ['Missing mode parameter']}, 422

        setmode = data['mode']
        if not setmode in ['start', 'stop']:
            return {"status": "error", "errors": ['Invalid maintmode: {m}'.format(m=setmode)]}, 422

        zcmd = None
        if setmode == 'start':
            zcmd = 'startMaintenance'
        elif setmode == 'stop':
            zcmd = 'stopMaintenance'
        else:
            return {'status': 'error', 'errors': ['Invalid maintmode']}, 422

        zresp = None
        try:
            zresp = aegir.zmq.prmessage(zcmd, None)
        except Exception as e:
            return {"status": "error", "errors": [str(e)]}, 422

        if zresp['status'] != 'success':
            return {'status': 'error', 'errors': [zresp['message']]}, 422

        return {"status": "success", "data": zresp['data']}, 200

    def post(self):
        data = flask.request.get_json();

        zdata = {}
        if 'mtpump' in data:
            zdata['mtpump'] = (data['mtpump'] == True)
            pass
        if 'bkpump' in data:
            zdata['bkpump'] = (data['bkpump'] == True)
            pass
        if 'heat' in data:
            zdata['heat'] = (data['heat'] == True)
            pass
        if 'temp' in data:
            zdata['temp'] = float(data['temp'])
            pass

        if len(zdata)==0:
            return {'status': 'error', 'errors': ['No fields set']}, 422

        #pprint([data,zdata])

        zresp = None
        try:
            zresp = aegir.zmq.prmessage('setMaintenance', zdata)
        except Exception as e:
            return {"status": "error", "errors": [str(e)]}, 422

        if zresp['status'] != 'success':
            return {'status': 'error', 'errors': [zresp['message']]}, 422

        return {"status": "success", "data": zresp['data']}, 200
    pass

class BrewOverride(flask_restful.Resource):
    def post(self):
        data = flask.request.get_json();
        zdata = {}

        for field in ['blockheat', 'forcemtpump', 'bkpump']:
            if field in data:
                zdata[field] = data[field]
                pass
            pass

        zresp = None
        #pprint([data, zdata])
        try:
            zresp = aegir.zmq.prmessage('override', zdata)
        except Exception as e:
            return {"status": "error", "errors": [str(e)]}, 422

        if zresp['status'] != 'success':
            return {'status': 'error', 'errors': [zresp['message']]}, 422

        return {"status": "success", "data": zresp['data']}, 200
    pass

class BrewStateCoolTemp(flask_restful.Resource):
    '''
    Setting the cooling target temperature during the cooling state
    '''

    def post(self):
        '''
        Sets a new cooling target temp
        '''
        data = flask.request.get_json();
        zcmd = 'setCoolTemp';

        if not 'cooltemp' in data:
            return {'status': 'error',
                    'errors': ['cooltemp missing']}, 422

        zdata = {'cooltemp': data['cooltemp']}
        try:
            zresp = aegir.zmq.prmessage(zcmd, zdata)
        except Exception as e:
            return {"status": "error", "errors": [str(e)]}, 422

        if zresp['status'] != 'success':
            if 'message' in zresp:
                return {"status": "error", "errors": [zresp['message']]}, 422
            return {"status": "error", "errors": ['Unknown error']}, 422

        return {'status': 'success'}
