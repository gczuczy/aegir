
DROP TABLE IF EXISTS programs;
CREATE TABLE programs (
       id integer primary key autoincrement,
       endtemp integer not null default 80,
       boiltime integer not null default 60,
       name varchar not null unique,
       check (endtemp > 70 and endtemp <= 95),
       check (boiltime >= 0 and boiltime < 300)
);

DROP TABLE IF EXISTS programs_tempsteps;
CREATE TABLE programs_tempsteps (
     id	   integer primary key autoincrement,
     progid integer not null,
     temperature integer not null,
     holdtime integer not null,
     FOREIGN KEY (progid) REFERENCES programs(id),
     CHECK (temperature > 20 and temperature <= 95),
     CHECK (holdtime > 1 and holdtime <= 180)
);

DROP TABLE IF EXISTS programs_hops;
CREATE TABLE programs_hops (
     id	   integer primary key autoincrement,
     progid integer not null,
     attime integer not null,
     hopname varchar not null,
     hopqty integer not null,
     FOREIGN KEY (progid) REFERENCES programs(id),
     CHECK (attime >= 0),
     CHECK (hopqty > 0)
);
