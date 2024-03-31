CREATE TABLE globals (
  name text PRIMARY KEY,
  value text NOT NULL
) STRICT;

INSERT INTO globals (name, value) VALUES ('version', '1');

CREATE TABLE fermenter_types (
  id integer PRIMARY KEY,
  name text NOT NULL,
  capacity int NOT NULL, -- liters
  imageurl text NOT NULL,
  CHECK (capacity>10 AND capacity < 300)
);

INSERT INTO fermenter_types (id, name, capacity, imageurl) VALUES
(1, 'SSBrewtech Chronical 2.0', 32, 'https://www.ssbrewtech.com/cdn/shop/products/7galChronical_800x.png'),
(2, 'SSBrewtech Chronical 2.0', 64, 'https://www.ssbrewtech.com/cdn/shop/products/7galChronical_800x.png'),
(3, 'SSBrewtech Chronical 2.0', 77, 'https://www.ssbrewtech.com/cdn/shop/products/7galChronical_800x.png'),
(4, 'SSBrewtech Unitank', 32, 'https://www.ssbrewtech.com/cdn/shop/products/Unitank2.07galhero_800x.jpg'),
(5, 'SSBrewtech Unitank', 64, 'https://www.ssbrewtech.com/cdn/shop/products/Unitank2.014.2galhero_800x.jpg'),
(6, 'SSBrewtech Unitank', 77, 'https://www.ssbrewtech.com/cdn/shop/products/Unitank2.017galhero_800x.jpg')
;

CREATE TABLE fermenters (
  id integer PRIMARY KEY,
  name text NOT NULL UNIQUE,
  typeid text NOT NULL,
  FOREIGN KEY (typeid) REFERENCES fermenter_types(id) ON DELETE RESTRICT
);

INSERT INTO fermenters (name, typeid) VALUEs
('Fermenter 1', 2),
('Fermenter 2', 2)
;

CREATE TABLE yeasts (
  id integer PRIMARY KEY,
  name text NOT NULL UNIQUE,
  attenuation real NOT NULL,
  abv real NOT NULL,
  mintemp real NOT NULL,
  maxtemp real NOT NULL,
  CHECK (mintemp < maxtemp ),
  CHECK (mintemp > 4 ),
  CHECK (maxtemp < 30),
  CHECK (attenuation > 10 AND attenuation <= 100 ),
  CHECK (abv > 4 AND abv < 30 )
);

INSERT INTO yeasts (name, attenuation, mintemp, maxtemp, abv) VALUES
('YF-101 Queen of hearts', 75, 17, 22, 10),
('YF-102 British Bulldog', 72, 18, 23, 10),
('YF-103 Steampunk', 75, 18, 23, 10),
('YF-104 Terrier', 71, 18, 22, 9),
('YF-105 Haggis', 73, 13, 23, 12),
('YF-106 Redshamrock', 75, 17, 22, 12),
('YF-107 McGyver', 77, 15, 22, 11),
('YF-109 Caskaway', 71, 18, 22, 9),
('YF-111 Pacification', 70, 18, 20, 10),
('YF-112 Vermonter', 82, 18, 22, 12),
('YF-202 Priest Dvel', 77, 18, 24, 12),
('YF-402 Herr Weizen', 77, 18, 24, 10),
('YF-666 Yeastman', 100, 18, 20, 25)
;

CREATE TABLE brews (
  id integer PRIMARY KEY,
  name text NOT NULL UNIQUE,
  yeastid int NOT NULL,
  brewdate text NOT NULL,
  originalsg real,
  metadata text,
  FOREIGN KEY (yeastid) REFERENCES yeasts (id) ON DELETE RESTRICT,
  CHECK (originalsg IS NULL OR (originalsg > 1.0 AND originalsg < 1.500))
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
  enabled int NOT NULL DEFAULT 0,
  fermenterid int,
  calibr_null real,
  calibr_at real,
  calibr_sg real,
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
