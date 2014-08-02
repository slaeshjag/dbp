#!/bin/bash
#dbp-template

# The "dbp-template" line must not be altered nor be moved, it must be the second
# line in the file for proper executable nuking when dbpd starts.


# Wrapper script that simulates a binary of the desired program inside $PATH
# For use with the dbp system

# anything surrounded by an ! followed by % is replaced with
# actual values by dbpd when the special folder is populated

if [ !%package_enviroment! == 1 ]; then
	# This is where pre/post launch env setup should be done

	union_dir="`dbp-cfg union_mount_dir`/!%package_id!"

	# chdir is done by dbp-run, as the union mount might not exist before
	# dbp-run is runned
	dbp-run !%package_id! !%package_binary! --env --args "$@"
else
	dbp-run !%package_id! !%package_binary! --args "$@"
fi
