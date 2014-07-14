#!/bin/bash

make vm-submit
mv vm-submit.tar.gz ../k3_backup/vm-`date +%Y.%m.%d`_`date +%H.%M.%S`.tar.gz
