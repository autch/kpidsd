
ZIPNAME=kpidop_$(shell date +%Y%m%d%H%M).zip

zip:
	mkdir -p kpidop/{x86,x64}
	cp ../Release/kpidop.kpi kpidop/x86/
	cp ../x64/Release/kpidop.kpi kpidop/x64/
	cp kpidop.txt kpidop/
	cp COPYING kpidop/
	zip -9r $(ZIPNAME) kpidop/
