
ZIPNAME=kpid2p_$(shell date +%Y%m%d%H%M).zip

LICENCES=COPYING COPYING.dsd2pcm COPYING.LGPL LICENCE.soxr

zip:
	mkdir -p kpid2p/{x86,x64}
	cp ../Release/kpid2p.kpi kpid2p/x86/
	cp ../x64/Release/kpid2p.kpi kpid2p/x64/
	cp kpid2p.txt kpid2p/
	cp $(LICENCES) kpid2p/
	zip -9r $(ZIPNAME) kpid2p/
