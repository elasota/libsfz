NINJA=ninja -C out/cur

all:
	@$(NINJA)

test: all
	out/cur/algorithm-test
	out/cur/args-test
	out/cur/bytes-test
	out/cur/digest-test
	out/cur/encoding-test
	out/cur/format-test
	out/cur/io-test
	out/cur/json-test
	out/cur/optional-test
	out/cur/os-test
	out/cur/print-test
	out/cur/read-test
	out/cur/string-test
	out/cur/string-utils-test
	out/cur/write-test

clean:
	@$(NINJA) -t clean

distclean:
	rm -Rf out/
	rm -f build/lib/scripts/gn

.PHONY: all test clean dist distclean
