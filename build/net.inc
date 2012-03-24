# ------------------------------------
# pion-net automake configuration file
# ------------------------------------

docs: doxygen-doc
	rm -rf common/doc/html
	ln -s ../../doc/html common/doc/html

clean-docs: doxygen-clean
	rm -rf common/doc/html
