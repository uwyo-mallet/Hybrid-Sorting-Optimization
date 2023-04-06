CC = pdflatex
FLAGS = --shell-escape -interaction=scrollmode --synctex=1
REPORT_JOB = report

.PHONY: tidy clean

all: report.pdf

report.pdf: report.tex references.bib figures/*.png
	$(CC) $(FLAGS) -jobname $(REPORT_JOB) report.tex
	biber $(REPORT_JOB)
	$(CC) $(FLAGS) -jobname $(REPORT_JOB) report.tex
	$(CC) $(FLAGS) -jobname $(REPORT_JOB) report.tex

tidy:
	rm -f *.aux \
          *.bbl \
          *.bcf \
          *.blg \
          *.log \
          *.nav \
          *.out \
          *.snm \
          *.toc \
          *.xml

clean: tidy
	find . -name "*.pdf" ! -name "README.pdf" -delete
	rm -rf .auctex-auto/
