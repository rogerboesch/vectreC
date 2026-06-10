# For help on the Markdown syntax: see
# http://daringfireball.net/projects/markdown/syntax

man_MANS = $(PACKAGE).1

MARKDOWN = markdown
PANDOC = pandoc

MANUAL = $(PACKAGE)-manual

HTML_DOC_FILES = $(MANUAL).html $(PACKAGE)-implementation.html $(PACKAGE)-vectrex.html

html-local: $(HTML_DOC_FILES)

$(MANUAL).html: $(MANUAL).markdown toc.pl
	$(MARKDOWN) $(MANUAL).markdown > $(MANUAL).html
	$(PERL) -i toc.pl $(MANUAL).html  # add table of contents in place
	grep '<H2>Table of Contents' $(MANUAL).html  # check that it worked

$(PACKAGE)-implementation.html: $(PACKAGE)-implementation.markdown toc.pl
	$(MARKDOWN) $(PACKAGE)-implementation.markdown > $(PACKAGE)-implementation.html
	$(PERL) -i toc.pl $(PACKAGE)-implementation.html  # add table of contents in place
	grep '<H2>Table of Contents' $(PACKAGE)-implementation.html  # check that it worked

$(PACKAGE)-vectrex.html: $(PACKAGE)-vectrex.markdown
	$(MARKDOWN) $(PACKAGE)-vectrex.markdown > $(PACKAGE)-vectrex.html


# N.B.: Generating PDF documents requires pandoc and pdflatex.

HTML_PDF_FILES = $(HTML_DOC_FILES:.html=.pdf)

pdf-local: $(HTML_PDF_FILES)

%.pdf: %.html
	$(PANDOC) $< -t latex -o $@


CLEANFILES = $(HTML_DOC_FILES) $(HTML_PDF_FILES)

EXTRA_DIST = \
	$(MANUAL).markdown \
	$(PACKAGE)-implementation.markdown \
	$(PACKAGE)-vectrex.markdown \
	toc.pl
