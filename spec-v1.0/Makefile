.PHONY : spec
spec:
	@-rm complete-spec.html
	pandoc frontmatter.md \
	dcp-version-contributors.md \
	dcp-changelog.md \
	dcp-context.md \
	dcp-datatypes.md \
	dcp-architecture.md \
	dcp-bp.md \
	dcp-srp.md \
	dcp-vardis.md \
	dcp-issues.md \
	--toc --number-sections --toc-depth=3 -s -o complete-spec.html


.PHONY : specpdf
specpdf:
	@-rm complete-spec.pdf
	pandoc frontmatter.md \
	dcp-version-contributors.md \
	dcp-changelog.md \
	dcp-context.md \
	dcp-datatypes.md \
	dcp-architecture.md \
	dcp-bp.md \
	dcp-srp.md \
	dcp-vardis.md \
	dcp-issues.md \
	--toc --number-sections --toc-depth=3 -s -o complete-spec.pdf

.PHONY : pspec
pspec:
	@-rm complete-spec.html
	pandoc frontmatter.md \
	dcp-bp.md \
	dcp-vardis.md \
	--toc --number-sections --toc-depth=3 -s -o complete-spec.html
