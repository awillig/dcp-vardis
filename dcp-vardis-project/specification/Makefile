
all : dcpvardis-spec.html dcpvardis-spec.pdf

MDFILES = frontmatter.md dcp-version-contributors.md dcp-context.md dcp-datatypes.md dcp-architecture.md dcp-bp.md dcp-srp.md dcp-vardis.md dcp-issues.md

TOCOPTS = --toc --toc-depth=3

dcpvardis-spec.html : $(MDFILES)
	@-rm $@
	pandoc $(MDFILES) $(TOCOPTS) --number-sections -s -t html -o $@

dcpvardis-spec.pdf : $(MDFILES)
	@-rm $@
	pandoc $(MDFILES) $(TOCOPTS) --number-sections -s -t pdf -o $@
