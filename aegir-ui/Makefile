
NG=node_modules/@angular/cli/bin/ng
NODE=/usr/local/bin/node

.PHONY: devserver clean

.MAIN: devserver

devserver: $(NG)
	$(NODE) --max_old_space_size=384 $(NG) serve --host 0.0.0.0 --disable-host-check 

clean:
	rm -rf $(MODULEDIR)

$(NG):
	npm install
