'''
Data handling, this is currently sqlite
'''

import sqlite3

import aegir.config

_conn = None

def init(app):
    app.before_first_request(instance_init)
    app.after_request(after_request)
    pass

def instance_init():
    global _conn
    _conn = sqlite3.connect(aegir.config.config['sqlitedb'])
    pass

def after_request(resp):
    if _conn:
        _conn.commit()
    return resp

def getprograms():
    global _conn

    ret = []

    for prog in _conn.execute('SELECT id,name FROM programs ORDER BY name'):
        ret.append(dict(prog))
        pass

    return ret
