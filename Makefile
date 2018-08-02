BUILDDIR = build
ARTIFACTDIR = artifacts
PROGRAMNAME = zstream_splitter

DEBBUILDDIR = ${BUILDDIR}/deb-build
DEBDIR = ${ARTIFACTDIR}/deb

ifndef DEBUILD_ARGS
    DEBUILD_ARGS=-us -uc
endif

.PHONY: all makedirs build tgz clean
all: build

clean:
	rm -rf ${BUILDDIR}/

makedirs:
	mkdir -p ${BUILDDIR} ${ARTIFACTDIR} ${DEBBUILDDIR} ${DEBDIR}

build: makedirs
	cd ${BUILDDIR} &&\
	cmake .. &&\
	make
	cp ${BUILDDIR}/${PROGRAMNAME} ${ARTIFACTDIR}/

tgz: makedirs build
	cd ${BUILDDIR} &&\
	make package
	cp ${BUILDDIR}/*.tar.gz ${ARTIFACTDIR}/

deb: clean makedirs
	rsync -ax --exclude ${ARTIFACTDIR} --exclude ${BUILDDIR} ./ ${DEBBUILDDIR}/
	cd ${DEBBUILDDIR} && debuild ${DEBUILD_ARGS}
	cp ${BUILDDIR}/*.* ${DEBDIR}/