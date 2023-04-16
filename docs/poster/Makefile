.PHONY: main clean FORCE

main: poster.pdf

poster.pdf: FORCE
	latexmk -bibtex -pdflatex='lualatex -interaction nonstopmode' -pdf poster.tex

clean:
	latexmk -pdf -C
