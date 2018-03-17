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

def getprogram(progid):
    global _conn

    curs = _conn.cursor()
    pres = curs.execute('SELECT id,name,starttemp,endtemp,boiltime,nomash,noboil FROM programs WHERE id=?', (progid,))
    prog = pres.fetchone()
    if prog == None:
        raise Exception('No program with ID {id}'.format(id = progid))

    for field in ['noboil', 'nomash']:
        prog[field] = prog[field] == 1
        pass

    msres = curs.execute('''
    SELECT id,orderno,temperature,holdtime
    FROM programs_mashsteps
    WHERE progid=?
    ORDER BY orderno''', (progid,))
    prog['mashsteps'] = []
    for step in msres:
        prog['mashsteps'].append(step)
        pass

    hopres = curs.execute('''
    SELECT id,attime, hopname, hopqty
    FROM programs_hops
    WHERE progid=?
    ORDER BY attime DESC
    ''', (progid,))
    prog['hops'] = []
    for hop in hopres:
        prog['hops'].append(hop)
        pass

    return prog

def checkprogramname(name, progid):
    global _conn

    curs = _conn.cursor()
    res = None
    if progid == None:
        res = curs.execute('SELECT count(*) AS c FROM programs WHERE name = ?', (name, ))
    else:
        res = curs.execute('SELECT count(*) AS c FROM programs WHERE name = ? AND NOT id = ?', (name, progid))
    if res.fetchone()['c'] != 0:
        return False
    return True

def addprogram(prog):
    global _conn

    pprint(prog)

    curs = _conn.cursor()
    curs.execute('''
    INSERT INTO programs (name, starttemp, endtemp, boiltime, nomash, noboil)
    VALUES (:name,:starttemp,:endtemp,:boiltime,:nomash, :noboil)''',
                 {'name': prog['name'],
                  'starttemp': prog['starttemp'],
                  'endtemp': prog['endtemp'],
                  'boiltime': prog['boiltime'] or 60,
                  'nomash': prog['nomash'],
                  'noboil': prog['noboil']})
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

    return progid

def saveprogram(prog):
    global _conn

    if not 'id' in prog:
        raise Exception('Program id not defined')

    progid = prog['id']
    curs = _conn.cursor()
    curs.execute('''
    UPDATE programs
    SET name=:name, starttemp=:starttemp, endtemp=:endtemp, boiltime=:boiltime,
        nomash=:nomash, noboil=:noboil
    WHERE id = :id''',
                 {'id': progid,
                  'name': prog['name'],
                  'starttemp': prog['starttemp'],
                  'endtemp': prog['endtemp'],
                  'boiltime': prog['boiltime'] or 60,
                  'nomash': prog['nomash'],
                  'noboil': prog['noboil']})

    # delete the associated mash steps and hops
    curs.execute('DELETE FROM programs_mashsteps WHERE progid = ?', (progid,))
    curs.execute('DELETE FROM programs_hops WHERE progid = ?', (progid,))
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

    return progid

def delprogram(progid):
    global _conn

    _conn.execute('DELETE FROM programs WHERE id = ?', (progid,));
    pass
