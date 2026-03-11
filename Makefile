TEX_FILES = $(wildcard exercise*.tex)
PDF_FILES = $(TEX_FILES:.tex=.pdf)

.PHONY: all
all: $(PDF_FILES)

%.pdf: %.tex
	pdflatex $<

.PHONY: clean
clean:
	rm -f *.aux *.log *.out *.toc *.fls *.fdb_latexmk *.synctex.gz *.pytxcode *.pdf
	rm -rf _minted*/