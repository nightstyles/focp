#!/bin/bash
for c in $@
{
	make -C $c $FOCP_SECTION
	if [ "$?" != "0" ]
	then
		break
	fi	
}
