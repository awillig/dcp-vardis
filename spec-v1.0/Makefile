
all : dcpvardis-v1-spec.html dcpvardis-v1-spec.pdf

MDFILES = frontmatter.md dcp-version-contributors.md dcp-context.md dcp-datatypes.md dcp-architecture.md dcp-bp.md dcp-srp.md dcp-vardis.md dcp-issues.md

TOCOPTS = --toc --toc-depth=3

dcpvardis-v1-spec.html : $(MDFILES)
	@-rm $@
	pandoc $(MDFILES) $(TOCOPTS) --number-sections -s -t html -o $@

dcpvardis-v1-spec.pdf : $(MDFILES)
	@-rm $@
	pandoc $(MDFILES) $(TOCOPTS) --number-sections -s -t pdf -o $@
