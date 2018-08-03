BUILDDIR = build
ARTIFACTDIR = artifacts
PROGRAMNAME = zstream-splitter



.PHONY: all makedirs build package clean
all: build

clean:
	rm -rf ${BUILDDIR}/

makedirs:
	mkdir -p ${BUILDDIR} ${ARTIFACTDIR}

build: makedirs
	cd ${BUILDDIR} &&\
	cmake .. &&\
	make
	cp ${BUILDDIR}/${PROGRAMNAME} ${ARTIFACTDIR}/

package: makedirs build
	cd ${BUILDDIR} &&\
	make package
	cp ${BUILDDIR}/${PROGRAMNAME}*.deb ${ARTIFACTDIR}/
	cp ${BUILDDIR}/${PROGRAMNAME}*.tar.gz ${ARTIFACTDIR}/
