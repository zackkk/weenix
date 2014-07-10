#!/bin/bash

make vfs-submit
mv vfs-submit.tar.gz ../k2_backup/procs-`date +%Y.%m.%d`_`date +%H.%M.%S`.tar.gz
