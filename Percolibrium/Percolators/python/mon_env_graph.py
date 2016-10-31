#!/usr/bin/env python

from lib.neo4j import Neo4j
import urllib, urllib2, json, sys, os, time, pprint, time, pyinotify, glob


config = {}
execfile("conf/config.conf", config)

neo4j = Neo4j(config['neo4j_url'], config['neo4j_user'], config['neo4j_pass'])
pp = pprint.PrettyPrinter(indent=4)
res = []
mylist = []

class ProcessTransientFile(pyinotify.ProcessEvent):
	def process_IN_MOVED_TO(self, event):
		# We have explicitely registered for this kind of event.
		print '\t', event.pathname, ' -> written'
		f = 'data/env_graph.%d' % (time.time())
		os.rename(event.pathname, f)
#		print time.strftime("%d/%m/%Y %H:%M:%S") + " - MOVING env_graph TO %s" % (f)

#	def process_default(self, event):
#		print 'default: ', event.maskname


def watch_files():
	wm = pyinotify.WatchManager()
	notifier = pyinotify.Notifier(wm)
	# In this case you must give the class object (ProcessTransientFile)
	# as last parameter not a class instance.
	wm.watch_transient_file('/home/ubuntu/.CGNgine/state/env_graph', pyinotify.ALL_EVENTS, ProcessTransientFile)
	notifier.loop()

watch_files()
