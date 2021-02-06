# makefile - build system for vscode extension building
#
# Just run `make` and it will update syntaces
#
# @author: Cade Brown <cade@kscript.org>

all: syntaxes/kscript.tmLanguage.json

.PHONY: all

syntaxes/%.json: syntaxes/%.yaml
	npx js-yaml $^ > $@
