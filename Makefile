all: zip

zip:
	for d in kpidop kpid2p kpidsd; do make -C $$d; done
