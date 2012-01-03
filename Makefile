all:
	@scons

clean:
	-rm -rf build

library:
	scons library
