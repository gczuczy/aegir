
NG=node_modules/@angular/cli/bin/ng.js
NODE=/usr/local/bin/node
MODULEDIR=node_modules
DISTDIR=dist

.PHONY: devserver clean

.MAIN: devserver

devserver: $(NG)
	$(NG) serve --host 0.0.0.0 --disable-host-check

build: $(NG)
	$(NG) build -c production

clean:
	rm -rf $(MODULEDIR) $(DISTDIR)

$(NG):
	npm install
