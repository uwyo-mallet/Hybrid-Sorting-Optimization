.PHONY: tidy clean

all: presentation.pdf

presentation.pdf: presentation.tex
	latexmk -shell-escape -bibtex -pdflatex='lualatex -interaction nonstopmode' -pdf presentation.tex

tidy:
	latexmk -pdf -CA

clean: tidy
	find . -name "*.pdf" ! -name "README.pdf" -delete
	rm -rf *.xml .auctex-auto/
