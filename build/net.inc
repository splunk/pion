# ------------------------------------
# pion-net automake configuration file
# ------------------------------------

docs: doxygen-doc
	rm -rf common/doc/html
	ln -s ../../net/doc/html common/doc/html
