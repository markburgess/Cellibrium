# KM-CFEngine 

This code is experimental research, using a software agent based on
CFEngine 3, a popular open source configuration management system,
whose primary function was to provide automated configuration and
maintenance of large-scale computer systems.

This fork is currently for concept development of a new form of
embedded management system, for the Internet of Things, Smart Spaces,
and Artificial Reasoning, which builds on the CFEngine legacy.
Because the refactored development of CFEngine has tied concepts,
methods, and libraries together in a complex mesh, it is difficult to
disentangle the new from the old at this stage, so I will not attempt
to do so as long as the code is considered experimental.


## Relationship to CFEngine 2 and 3

KM-CFEngine is *not* a drop-in upgrade for CFEngine 2 installations.
It is incompatible with the CFEngine 2 policy language.

KM-CFEngine is a drop-in replacement for some versions of CFEngine 3,
but diverges from the CFEngine core at around version 3.6.


MB, August 2016
