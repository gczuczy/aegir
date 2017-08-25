
NG=node_modules/@angular/cli/bin/ng

.PHONY: devserver clean

.MAIN: devserver

devserver: $(NG)
	$(NG) serve --host 0.0.0.0 --disable-host-check

clean:
	rm -rf $(MODULEDIR)

$(NG):
	npm install
