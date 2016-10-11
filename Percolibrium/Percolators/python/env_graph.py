#!/usr/bin/env python

from lib.neo4j import Neo4j
import urllib, urllib2, json, sys, os, time, pprint


config = {}
execfile("conf/config.conf", config)

neo4j = Neo4j(config['neo4j_url'], config['neo4j_user'], config['neo4j_pass'])
pp = pprint.PrettyPrinter(indent=4)
res = []
mylist = []
with open('data/env_graph') as f:
	mylist = [ i.rstrip('\n').lstrip('(').rstrip(')').split(',') for i in f ]
	#res.append(mylist)

statements = []
for l in mylist:
	statements.append({
		'statement': (
			'MERGE (n:Label {Name: {one}})'
			'MERGE (m:Label {Name: {two}})'
			#'MERGE (n)-[:`%s` {type: %s, b:`%s`}]-(m)' % (l[2],l[1],l[3])
#			'MERGE (n)-[:`%s` {type:{three}]-(m)' % (l[2])
			'MERGE (n)-[:`%s` {type:{three}, b: {four}}]->(m)' % (l[2])
		#	'MERGE (m)-[:`%s` {type:{three}, b: {two}}]->(m)' % (l[4]) 
		),
		'parameters': {
			'one': l[0],
			'two': l[4],
			'three' : l[1],
			'four' : l[3],
		}
	})

r = neo4j.neo4j_rest_transaction_commit({'statements': statements})
