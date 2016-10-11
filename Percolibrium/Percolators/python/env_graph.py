#!/usr/bin/env python

from lib.neo4j import Neo4j
from multiprocessing import Process
import urllib, urllib2, json, sys, os, time, pprint, time, pyinotify


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
		os.rename(event.pathname, 'data/env_graph.%d' % (time.time()))

	def process_default(self, event):
		# Implicitely IN_CREATE and IN_DELETE are watched too. You can
		# ignore them and provide an empty process_default or you can
		# process them, either with process_default or their dedicated
		# method (process_IN_CREATE, process_IN_DELETE) which would
		# override process_default.
		print 'default: ', event.maskname


def watch_files():
	wm = pyinotify.WatchManager()
	notifier = pyinotify.Notifier(wm)
	# In this case you must give the class object (ProcessTransientFile)
	# as last parameter not a class instance.
	wm.watch_transient_file('/home/ubuntu/.CGNgine/state/env_graph', pyinotify.ALL_EVENTS, ProcessTransientFile)
	notifier.loop()

def insert_into_db():
	while True:
		with open('data/env_graph') as f:
			mylist = [ i.rstrip('\n').lstrip('(').rstrip(')').split(',') for i in f ]
			#res.append(mylist)

		statements = []
		for l in mylist:
			if l[1] < 0:
				l[1] *= -1
				merge1 = 'MERGE (n)<-[:`%s` {type:{two}, b: {three}}]-(m)' % (l[4])
				merge2 = 'MERGE (n)-[:`%s` {type:{two}, b: {five}}]->(m)' % (l[2])
			else:
				merge1 = 'MERGE (n)-[:`%s` {type:{two}, b: {five}}]->(m)' % (l[2])
				merge2 = 'MERGE (n)<-[:`%s` {type:{two}, b: {three}}]-(m)' % (l[4])

			statements.append({
			'statement': (
#				'MERGE (n:Label {Name: {one}})'
#				'MERGE (m:Label {Name: {three}})'
				'MERGE (n {Name: {one}})'
				'MERGE (m {Name: {four}})'
				'%s %s' % (merge1, merge2)
				#'MERGE (n)-[:`%s` {type: %s, b:`%s`}]-(m)' % (l[2],l[1],l[3])
	#			'MERGE (n)-[:`%s` {type:{three}]-(m)' % (l[2])
			#	'MERGE (m)-[:`%s` {type:{three}, b: {two}}]->(m)' % (l[4]) 
			),
			'parameters': {
				'one': l[0],
				'two': l[1],
				'three' : l[2],
				'four' : l[3],
				'five' : l[4],
			}
		})
		pp.pprint(statements)
		print time.strftime("%d/%m/%Y %H:%M:%S") + " - STARTING LOAD"
		r = neo4j.neo4j_rest_transaction_commit({'statements': statements})
		print time.strftime("%d/%m/%Y %H:%M:%S") + " - COMPLETED LOAD"
		time.sleep(60)


p = Process(target=insert_into_db)
d = Process(target=watch_files)
p.start()
d.start()
print "calling thread"
p.join()
p.join()
