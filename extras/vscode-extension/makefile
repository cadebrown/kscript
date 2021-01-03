

all: syntaxes/kscript.tmLanguage.json

.PHONY: all

syntaxes/%.json: syntaxes/%.yaml
	npx js-yaml $^ > $@
