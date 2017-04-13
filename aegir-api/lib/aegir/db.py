'''
Data handling, this is currently sqlite
'''

import sqlite3
from pprint import pprint

import aegir.config

_conn = None

def init(app):
    app.before_first_request(instance_init)
    app.after_request(after_request)
    pass

def dict_factory(cursor, row):
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
        pass
    return d

def instance_init():
    global _conn
    _conn = sqlite3.connect(aegir.config.config['sqlitedb'])
    _conn.row_factory = dict_factory
    pass

def after_request(resp):
    if _conn:
        _conn.commit()
    return resp

def getprograms():
    global _conn

    ret = []

    for prog in _conn.execute('SELECT id,name FROM programs ORDER BY name'):
        ret.append(prog)
        pass

    return ret

def checkprogramname(name):
    global _conn

    curs = _conn.cursor()
    res = curs.execute('SELECT count(*) AS c FROM programs WHERE name = ?', (name, ))
    if res.fetchone()['c'] != 0:
        return False
    return True

def addprogram(prog):
    global _conn

    curs = _conn.cursor()
    curs.execute('INSERT INTO programs (name, starttemp, endtemp, boiltime) VALUES (?,?,?,?)',
                 (prog['name'], prog['starttemp'], prog['endtemp'], prog['boiltime']))
    progid = curs.lastrowid

    # now add the mashsteps
    for step in prog['mashsteps']:
        curs.execute('''
        INSERT INTO programs_mashsteps (progid, orderno, temperature, holdtime)
        VALUES (?, ?, ?, ?)
        ''', (progid, step['order'], step['temp'], step['holdtime']))
        pass

    # and the hops
    for hop in prog['hops']:
        curs.execute('INSERT INTO programs_hops (progid, attime, hopname, hopqty) VALUES (?,?,?,?)',
                 (progid, hop['attime'], hop['name'], hop['quantity']))
        pass
    pass
