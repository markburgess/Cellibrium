# Generating example data. Start here

INCLUDES = -I. -I../../RobIoTs/CGNgine/libpromises -I../../RobIoTs/CGNgine/libutils -I../../RobIoTs/CGNgine/libcfnet

all: update build generate graph

update:
# CGNgine data, generated internally
	cp ~/.CGNgine/state/env_graph ExampleTupleData/
	cp ~/.CGNgine/policy_graph ExampleTupleData/
build:
	gcc $(INCLUDES) -o gen_bdk -g generate_binary_domain_knowledge.c
	gcc $(INCLUDES) -o gen_dsdk -g generate_doctor_domain_knowledge.c
	gcc $(INCLUDES) -o gen_cgndk -g generate_cgn_domain_knowledge.c
	gcc $(INCLUDES) -o gen_msdk -g generate_microservice_domain_knowledge.c
	gcc -o conceptualize -g conceptualize-fs.c

generate: microservice doctor lddscan monitord

lddscan:
	./gen_bdk > ExampleTupleData/dependencies_graph
doctor:
	./gen_dsdk > ExampleTupleData/doctorservice_graph
monitord:
	./gen_cgndk > ExampleTupleData/domain_knowledge_graph
microservice:
	./gen_msdk > ExampleTupleData/microservice_component_graph
graph:
	./conceptualize ExampleTupleData/*

# The go to ../Narrators for search