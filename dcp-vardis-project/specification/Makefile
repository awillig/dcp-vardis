
all : dcpvardis-spec.html dcpvardis-spec.pdf

MDFILES = frontmatter.md dcp-version-contributors.md dcp-changelog.md dcp-context.md dcp-datatypes.md dcp-architecture.md dcp-bp.md dcp-srp.md dcp-vardis.md dcp-issues.md

TOCOPTS = --toc --toc-depth=2

PDOPTS = -V colorlinks=true \
	 -V linkcolor=blue \
	 -V urlcolor=red \
	 -V toccolor=gray \
         -V documentclass=scrartcl \
         -V pagestyle=headings \
         -V fontsize=10pt \
         -V marginleft=1.3cm \
         -V marginright=1.3cm \
         -V margintop=1.3cm \
         -V marginbottom=1.3cm

dcpvardis-spec.html : $(MDFILES) Makefile
	@-rm $@
	pandoc $(MDFILES) $(PDOPTS) $(TOCOPTS) --number-sections -s -t html -o $@

dcpvardis-spec.pdf : $(MDFILES) Makefile
	@-rm $@
	pandoc $(MDFILES) $(PDOPTS) $(TOCOPTS) --number-sections -s -t pdf -o $@
