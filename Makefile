VERSION = 0.1.9

VIDEOSRC = ./videoanalysis/src/
AUDIOSRC = ./audioanalysis/src/

all:
	[ -d build ] || mkdir build
	$(MAKE) -C $(VIDEOSRC)
	$(MAKE) -C $(AUDIOSRC)

clean:
	$(MAKE) clean -C $(VIDEOSRC)
	$(MAKE) clean -C $(AUDIOSRC)

install:
	cp ./build/libvideoanalysis.so /usr/lib64/gstreamer-1.0
	cp ./build/libaudioanalysis.so /usr/lib64/gstreamer-1.0

uninstall:
	rm /usr/lib64/gstreamer-1.0/libvideoanalysis.so
	rm /usr/lib64/gstreamer-1.0/libaudioanalysis.so

tarboll:
	tar czvf /tmp/ats-analyzer-$(VERSION).tar.gz ../ats-plugins
	mv /tmp/ats-analyzer-$(VERSION).tar.gz ./
