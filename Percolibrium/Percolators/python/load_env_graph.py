#!/usr/bin/env python

from lib.neo4j import Neo4j
import urllib, urllib2, json, sys, os, time, pprint, time, pyinotify, glob


config = {}
execfile("conf/config.conf", config)

neo4j = Neo4j(config['neo4j_url'], config['neo4j_user'], config['neo4j_pass'])
pp = pprint.PrettyPrinter(indent=4)

def insert_into_db():
	while True:
		mylist = []
		nofiles = 0
		for fn in glob.glob('data/env_graph.[0-9]*'):
			nofiles += 1
			with open(fn) as f:
				l = [ i.rstrip('\n').lstrip('(').rstrip(')').split(',') for i in f ]
				mylist.extend(l)
				f.close()
				os.remove(fn)
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
				'MERGE (n:CGN {Name: {one}})'
				'MERGE (m:CGN {Name: {four}})'
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

insert_into_db()

