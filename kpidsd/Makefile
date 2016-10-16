
ZIPNAME=kpidsd_DEBUG_$(shell date +%Y%m%d%H%M).zip

zip:
	mkdir -p kpidsd/{x86,x64}
	cp ../Debug/kpidsd.kpi kpidsd/x86/
	cp ../x64/Debug/kpidsd.kpi kpidsd/x64/
	cp kpidsd.txt kpidsd/
	zip -9r $(ZIPNAME) kpidsd/
