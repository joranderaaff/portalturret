all: portal_turret_webpage/dist_compiled/index.html.gz.h

portal_turret_webpage:
	git clone https://github.com/joranderaaff/portal_turret_webpage.git

portal_turret_webpage/node_modules: portal_turret_webpage
	cd portal_turret_webpage; npm i

portal_turret_webpage/dist_compiled/index.html.gz.h: portal_turret_webpage/node_modules
	cd portal_turret_webpage; npm run build

distclean:
	rm -rf portal_turret_webpage