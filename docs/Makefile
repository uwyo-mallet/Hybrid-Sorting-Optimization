.PHONY: tidy clean

all: report.pdf

report.pdf: report.tex references.bib figures/*.png
	latexmk -bibtex -pdflatex='lualatex -interaction nonstopmode' -pdf report.tex

tidy:
	latexmk -pdf -CA

clean: tidy
	find . -name "*.pdf" ! -name "README.pdf" -delete
	rm -rf *.xml .auctex-auto/
