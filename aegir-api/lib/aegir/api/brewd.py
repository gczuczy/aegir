'''
This endpoint is responsible for communicating with the brewd backend over ZMQ
'''

import flask
import flask_restful
import dateutil.parser
import datetime
from pprint import pprint

import aegir.db
import aegir.zmq

def init(app, api):
    api.add_resource(LoadProgram, '/api/brewd/loadProgram')
    pass


class LoadProgram(flask_restful.Resource):
    def post(this):
        data = flask.request.get_json();

        program = None
        try:
            program = aegir.db.getprogram(data['id'])
        except Exception as e:
            return {"status": "error",
                    "errors": [str(e)]}, 422

        zreq = {"program": program}
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

        #pprint(['loading', zreq])
        zresp = None
        try:
            zresp = aegir.zmq.prmessage("loadProgram", zreq)
        except Exception as e:
            return {"status": "error",
                    "errors": [str(e)]}, 422
        #pprint(['zresp', zresp])

        return {"status": "success"}
    pass
