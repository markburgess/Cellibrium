#!/usr/bin/env python

from lib.neo4j import Neo4j
from multiprocessing import Process
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
		#print '\t', event.pathname, ' -> written'
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

def insert_into_db():
	while True:
		mylist = []
		nofiles = 0
		for fn in glob.glob('data/env_graph.[0-9]*'):
			nofiles += 1
			with open(fn) as f:
				l = [ i.rstrip('\n').lstrip('(').rstrip(')').split(',') for i in f ]
				mylist.extend(l)
				os.remove(fn)
			#res.append(mylist)
		statements = []
		for l in mylist:
			timestamp = time.time()
			r1_left = r2_right = '-'
			r1_right = '->'
			r2_left = '<-'

			if l[1] < 0:
				l[1] *= -1
			#	merge1 = 'MERGE (n)<-[r1:`%s` {type:{two}, b: {three}}]-(m) SET r1 += {r1_props}' % (l[4])
			#	merge2 = 'MERGE (n)-[r2:`%s` {type:{two}, b: {five}}]->(m) SET r2 += {r2_props}' % (l[2])
				r1_left = '<-'
				r1_right = r2_left = '-'
				r2_right = '->'

			merge1 = (
				'MERGE (n)%s[r1:`%s` {type:{two}, b: {five}}]%s(m) ON CREATE SET r1.w = 0.3, r1.ts = {ts} ' 
				'ON MATCH SET r1.ts = {ts}, r1.w = (r1.w + 0.7)'
			) % (r1_left,l[2],r1_right)

			merge2 = (
				'MERGE (n)%s[r2:`%s` {type:{two}, b: {three}}]%s(m) ON CREATE SET r2.w = 0.3, r2.ts = {ts} ' 
				'ON MATCH SET r2.ts = {ts}, r2.w = (r2.w + 0.7)'
			) % (r2_left,l[4],r2_right)

				#MERGE (n:TESTING {Name: 'testing1'})-[r:TESTING {Name: 'testing2'}]->(m:TESTING {Name: 'testing3'}) ON CREATE SET r.weight = 0.3, r.ts = 'blargh' ON MATCH SET r += {ts: '1234512qwerqwer3', weight: (r.weight + 0.7)};

			statements.append({
			'statement': (
#				'MERGE (n:Label {Name: {one}})'
#				'MERGE (m:Label {Name: {three}})'
				'MERGE (n:CGN {Name: {one}}) '
				'MERGE (m:CGN {Name: {four}}) '
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
				'ts': str(timestamp),
				}
		})
		#pp.pprint(statements)
		print time.strftime("%d/%m/%Y %H:%M:%S") + " - STARTING LOAD (%d files)" % (nofiles)
		r = neo4j.neo4j_rest_transaction_commit({'statements': statements})
		print time.strftime("%d/%m/%Y %H:%M:%S") + " - COMPLETED LOAD"
		time.sleep(5)


# Create index on CGN

q = {
	'statements': 
	[
		{
			'statement': 'CREATE INDEX ON :CGN(Name)'
		}
	]
}


r = neo4j.neo4j_rest_transaction_commit(q)

time.sleep(5)

p = Process(target=insert_into_db)
d = Process(target=watch_files)
p.start()
d.start()

print "calling thread"
p.join()
d.join()
