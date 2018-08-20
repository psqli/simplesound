#!/bin/bash

# Allow an executable to set scheduler parameters when run
# by an unprivileged user.
#
# This was made to use in waveplay when DEADLINE_WAKEUP is
# being used.

if [[ ! -x $1 ]]; then
	echo "Error, \"$1\" is not an executable"
	exit 1
fi

echo "Type root password"
su --command "setcap cap_sys_nice+ep $1"
