WP_FLAGS=--progress

.if defined(ENV) && $(ENV) == "PROD"
WP_FLAGS+= -p
.endif

.if defined(ENV) && $(ENV) == "DEV"
WP_FLAGS+= --watch
.endif

js: root/aegir.js

root/aegir.js: src/node_modules
	webpack $(WP_FLAGS)

clean:
	rm -rf src/node_modules
	rm -f root/aegir.js

src/node_modules:
	cd src; npm install
