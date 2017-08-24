
NG=node_modules/@angular/cli/bin/ng

.PHONY: devserver npm-install-modules clean

devserver:
	$(NG) serve --host 0.0.0.0

clean:
	rm -rf $(MODULEDIR)

npm-install-modules:
	npm install

$(NG): npm-install-modules
