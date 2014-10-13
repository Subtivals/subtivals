default: html

env:
	virtualenv env
	env/bin/pip install landslide

html: env
	env/bin/landslide -c landslide.cfg
