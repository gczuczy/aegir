CREATE TABLE globals (
  name text PRIMARY KEY,
  value text NOT NULL
) STRICT;

INSERT INTO globals (name, value) VALUES ('version', '1');

CREATE TABLE fermenters (
  id integer PRIMARY KEY,
  name text NOT NULL UNIQUE,
  type text NOT NULL,
  imageurl text
);

CREATE TABLE yeasts (
  id integer PRIMARY KEY,
  name text NOT NULL UNIQUE,
  attenuation real NOT NULL,
  mintemp real NOT NULL,
  maxtemp real NOT NULL,
  CHECK (mintemp < maxtemp ),
  CHECK (mintemp > 4 ),
  CHECK (maxtemp < 25),
  CHECK (attenuation > 10 AND attenuation < 100 )
);

CREATE TABLE brews (
  id integer PRIMARY KEY,
  name text NOT NULL UNIQUE,
  yeastid int NOT NULL,
  brewdate text NOT NULL,
  originalsg real NOT NULL,
  FOREIGN KEY (yeastid) REFERENCES yeasts (id) ON DELETE RESTRICT,
  CHECK (originalsg > 1.0 AND originalsg < 1.500)
);

CREATE TABLE fermentations (
  id integer PRIMARY KEY,
  brewid int NOT NULL,
  fermenterid int NOT NULL,
  transferdate text NOT NULL,
  FOREIGN KEY (brewid) REFERENCES brews(id) ON DELETE RESTRICT,
  FOREIGN KEY (fermenterid) REFERENCES fermenters(id) ON DELETE RESTRICT
);

CREATE TABLE tilthydrometers (
  id integer PRIMARY KEY,
  color text NOT NULL,
  uuid text NOT NULL,
  active int NOT NULL DEFAULT 0,
  enabled int NOT NULL DEFAULT 0,
  fermenterid int,
  calibr_null real,
  calibr_at real,
  calibr_sg real,
  CHECK (active == 0 OR active == 1),
  CHECK (enabled == 0 OR enabled == 1),
  FOREIGN KEY (fermenterid) REFERENCES fermenters(id) ON DELETE SET NULL,
  CHECK ( (calibr_null IS NULL) OR
  (calibr_null IS NOT NULL AND calibr_null > 0.990 AND calibr_null < 1.500)),
  CHECK ((calibr_at IS NULL AND calibr_sg IS NULL) OR
  (calibr_at IS NOT NULL AND calibr_sg IS NOT NULL AND
  calibr_at > 1.020 AND calibr_at < 1.500 AND
  calibr_sg > 1.010 AND calibr_sg < 1.500))
);

INSERT INTO tilthydrometers (uuid, color)
VALUES ('a495bb10-c5b1-4b44-b512-1370f02d74de', 'Red');
INSERT INTO tilthydrometers (uuid, color)
VALUES ('a495bb20-c5b1-4b44-b512-1370f02d74de', 'Green');
INSERT INTO tilthydrometers (uuid, color)
VALUES ('a495bb30-c5b1-4b44-b512-1370f02d74de', 'Black');
INSERT INTO tilthydrometers (uuid, color)
VALUES ('a495bb40-c5b1-4b44-b512-1370f02d74de', 'Purple');
INSERT INTO tilthydrometers (uuid, color)
VALUES ('a495bb50-c5b1-4b44-b512-1370f02d74de', 'Orange');
INSERT INTO tilthydrometers (uuid, color)
VALUES ('a495bb60-c5b1-4b44-b512-1370f02d74de', 'Blue');
INSERT INTO tilthydrometers (uuid, color)
VALUES ('a495bb70-c5b1-4b44-b512-1370f02d74de', 'Yellow');
INSERT INTO tilthydrometers (uuid, color)
VALUES ('a495bb80-c5b1-4b44-b512-1370f02d74de', 'Pink');
