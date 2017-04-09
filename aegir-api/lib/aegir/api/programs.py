'''
Listing of programs, and defining new ones
'''

import flask_restful

import aegir.db

def init(app, api):
    api.add_resource(Programs, '/api/programs')
    pass

class Programs(flask_restful.Resource):
    def get(self):
        return {'status': 'success',
                'data': aegir.db.getprograms()}
    pass
