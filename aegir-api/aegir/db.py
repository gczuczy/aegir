'''
Data handling, this is currently sqlite
'''

import sqlite3
import threading
from pprint import pprint

import aegir.config

dbconn = None

def init(app):
    #app.before_first_request(instance_init)
    app.after_request(after_request)
    pass

def dict_factory(cursor, row):
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
        pass
    return d

def instance_init():
    pass

def after_request(resp):
    global dbconn
    if dbconn is not None:
        dbconn.commit()
        pass
    #pprint(['after_request', threading.get_ident(), conn])
    return resp

class Connection():
    def __init__(self):
        global dbconn
        self._conn = sqlite3.connect(aegir.config.config['sqlitedb'])
        self._conn.row_factory = dict_factory
        dbconn = self
        #pprint(['db.Connection', threadid, self, self._conn])
        pass

    def commit(self):
        self._conn.commit()
        pass

    def execute(self, *args, **kwargs):
        return self._conn.execute(*args, **kwargs)

    def cursor(self):
        return self._conn.cursor()

    def getPrograms(self):
        ret = list()
        for prog in self._conn.execute('SELECT * FROM programs ORDER BY name'):
            ret.append(Program(self, data=prog))
            pass
        return ret

    def getProgram(self, progid):
        curs = self._conn.cursor()
        pres = curs.execute('SELECT id,name,starttemp,endtemp,boiltime,nomash,noboil FROM programs WHERE id=?', (progid,))
        prog = pres.fetchone()
        if prog is None:
            raise Exception('No program with ID {id}'.format(id = progid))
        return Program(self, data = prog);

    def checkProgramName(self, name, progid):
        curs = self._conn.cursor()
        res = None
        if progid == None:
            res = curs.execute('SELECT count(*) AS c FROM programs WHERE name = ?', (name, ))
        else:
            res = curs.execute('SELECT count(*) AS c FROM programs WHERE name = ? AND NOT id = ?', (name, progid))
            pass
        if res.fetchone()['c'] != 0:
            return False
        return True

    def addProgram(self, prog):
        curs = self._conn.cursor()
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
        if not prog['nomash']:
            for step in prog['mashsteps']:
                curs.execute('''
                INSERT INTO programs_mashsteps (progid, orderno, temperature, holdtime)
                VALUES (?, ?, ?, ?)
                ''', (progid, step['order'], step['temp'], step['holdtime']))
                pass
            pass

        # and the hops
        if not prog['noboil']:
            for hop in prog['hops']:
                curs.execute('INSERT INTO programs_hops (progid, attime, hopname, hopqty) VALUES (?,?,?,?)',
                             (progid, hop['attime'], hop['name'], hop['quantity']))
                pass
            pass

        return progid
    pass

class Program():
    def __init__(self, conn, _id=None, data=None):
        self._conn = conn
        curs = self._conn.cursor()

        # fill self._data
        if data is not None:
            self._data = data
        elif _id is not None :
            res = curs.execute('SELECT id,name,starttemp,endtemp,boiltime,nomash,noboil FROM programs WHERE id=?', (_id,))
            self._data = pres.fetchone()
            if self._data is None:
                raise Exception('No program with ID {id}'.format(id = _id))
        else:
            raise Exception('Either data or _id is needed')
            pass

        self._progid = self._data['id']

        # fetch the mashing steps
        msres = curs.execute('''
        SELECT id,orderno,temperature,holdtime
        FROM programs_mashsteps
        WHERE progid=?
        ORDER BY orderno''', (self._progid,))
        self._data['mashsteps'] = list()
        for step in msres:
            self._data['mashsteps'].append(step)
            pass

        # fetch the hopping steps
        hopres = curs.execute('''
        SELECT id,attime, hopname, hopqty
        FROM programs_hops
        WHERE progid=?
        ORDER BY attime DESC
        ''', (self._progid,))
        self._data['hops'] = []
        for hop in hopres:
            self._data['hops'].append(hop)
            pass

        # fill all the fields
        for field in ['noboil', 'nomash']:
            setattr(self, '_'+field, self._data[field] == 1)
            pass
        for field in ['name', 'starttemp', 'endtemp', 'boiltime', 'mashsteps', 'hops']:
            setattr(self, '_'+field, self._data[field])
            pass

        pass

    @property
    def progid(self):
        return self._progid

    @property
    def data(self):
        return self._data

    @property
    def noboil(self):
        return self._noboil

    @property
    def nomash(self):
        return self._nomash

    @property
    def name(self):
        return self._name

    @property
    def starttemp(self):
        return self._starttemp

    @property
    def endtemp(self):
        return self._endtemp

    @property
    def boiltime(self):
        return self._boiltime

    @property
    def mashsteps(self):
        return self._mashsteps

    @property
    def hops(self):
        return self._hops

    def update(self, prog):
        progid = self._progid
        curs = self._conn.cursor()

        # update the main data
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
        pass

    def remove(self):
        self._conn.execute('DELETE FROM programs WHERE id = ?', (self._progid,));
        pass

    pass
