
DROP TABLE IF EXISTS programs;
CREATE TABLE programs (
       id integer primary key autoincrement,
       starttemp integer not null default 37,
       endtemp integer not null default 80,
       boiltime integer not null default 60,
       name varchar not null unique,
       nomash integer not null default 0,
       noboil integer not null default 0,
       check (endtemp > 70 and endtemp <= 95),
       check (boiltime >= 0 and boiltime < 300),
       check (nomash = 0 or nomash = 1),
       check (noboil = 0 or noboil = 1),
       check (not (nomash and noboil))
);

DROP TABLE IF EXISTS programs_mashsteps;
CREATE TABLE programs_mashsteps (
     id	   integer primary key autoincrement,
     progid integer not null,
     orderno integer not null,
     temperature integer not null,
     holdtime integer not null,
     FOREIGN KEY (progid) REFERENCES programs(id) ON DELETE CASCADE,
     CHECK (temperature > 20 and temperature <= 95),
     CHECK (holdtime > 1 and holdtime <= 180),
     CHECK (orderno >= 0)
);

DROP TABLE IF EXISTS programs_hops;
CREATE TABLE programs_hops (
     id	   integer primary key autoincrement,
     progid integer not null,
     attime integer not null,
     hopname varchar not null,
     hopqty integer not null,
     FOREIGN KEY (progid) REFERENCES programs(id) ON DELETE CASCADE,
     CHECK (attime >= 0),
     CHECK (hopqty > 0)
);
