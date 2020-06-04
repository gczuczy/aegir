
NG=node_modules/@angular/cli/bin/ng
NODE=/usr/local/bin/node
MODULEDIR=node_modules
DISTDIR=dist

.PHONY: devserver clean

.MAIN: devserver

devserver: $(NG)
	$(NODE) $(NG) serve --host 0.0.0.0 --disable-host-check

build: $(NG)
	$(NG) build --prod

clean:
	rm -rf $(MODULEDIR) $(DISTDIR)

$(NG):
	npm install
