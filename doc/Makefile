DOC=mgr

all: latex show
latex:
	latex ${DOC}.tex && dvipdf ${DOC}.dvi
show:
	xpdf -z width ${DOC}.pdf
clean:
	rm -f ${DOC}.pdf *.aux *.log *.dvi *.out
