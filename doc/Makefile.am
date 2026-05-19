# For help on the Markdown syntax: see
# http://daringfireball.net/projects/markdown/syntax

man_MANS = $(PACKAGE).1

MARKDOWN = markdown
PANDOC = pandoc
PDFLATEX = pdflatex

MANUAL = $(PACKAGE)-manual

HTML_DOC_FILES = $(MANUAL).html $(PACKAGE)-implementation.html $(PACKAGE)-vectrex.html

html-local: $(HTML_DOC_FILES)

$(MANUAL).html: $(MANUAL).markdown toc.pl
	$(MARKDOWN) $(MANUAL).markdown > $(MANUAL).html
	$(PERL) -i toc.pl $(MANUAL).html  # add table of contents in place
	grep '<H2>Table of Contents' $(MANUAL).html  # check that it worked

# Generates a LaTeX standalone document.
# Requires the pandoc command.
# fix-tex.pl adds \tableofcontents to the .tex file and gives it a
# LaTeX \maketitle command, so that the start of the document only
# looks semi-ugly.
#
$(MANUAL).tex: $(MANUAL).markdown fix-tex.pl
	$(PANDOC) --standalone -f markdown -t latex -o $(MANUAL).tex $(MANUAL).markdown
	$(PERL) -i fix-tex.pl $(MANUAL).tex

.PHONY: tex
tex: $(MANUAL).tex

# Generates a PDF. 
# Requires the pandoc and pdflatex commands.
# pdflatex is run twice so that \tableofcontents gets replaced with an actual
# table of contents.
#
$(MANUAL).pdf: $(MANUAL).tex
	$(PDFLATEX) $(MANUAL).tex
	$(PDFLATEX) $(MANUAL).tex

.PHONY: pdf
pdf: $(MANUAL).pdf

$(PACKAGE)-implementation.html: $(PACKAGE)-implementation.markdown toc.pl
	$(MARKDOWN) $(PACKAGE)-implementation.markdown > $(PACKAGE)-implementation.html
	$(PERL) -i toc.pl $(PACKAGE)-implementation.html  # add table of contents in place
	grep '<H2>Table of Contents' $(PACKAGE)-implementation.html  # check that it worked

$(PACKAGE)-vectrex.html: $(PACKAGE)-vectrex.markdown
	$(MARKDOWN) $(PACKAGE)-vectrex.markdown > $(PACKAGE)-vectrex.html

PDF_DOC_FILES = \
	$(MANUAL).aux \
	$(MANUAL).log \
	$(MANUAL).out \
	$(MANUAL).tex \
	$(MANUAL).pdf

CLEANFILES = $(HTML_DOC_FILES) $(PDF_DOC_FILES)

EXTRA_DIST = \
	$(MANUAL).markdown \
	$(PACKAGE)-implementation.markdown \
	$(PACKAGE)-vectrex.markdown \
	toc.pl \
	fix-tex.pl
