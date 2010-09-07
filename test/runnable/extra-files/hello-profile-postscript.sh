#!/bin/bash

# strip out Dmain since it's symbol differs between windows and non-windows
grep -v Dmain ${RESULTS_DIR}/runnable/hello-profile.d.trace.def > ${RESULTS_DIR}/runnable/hello-profile.d.trace.def2

diff -w runnable/extra-files/hello-profile.d.trace.def ${RESULTS_DIR}/runnable/hello-profile.d.trace.def2
if [ $? -ne 0 ]; then
    exit 1;
fi

tracelog=${RESULTS_DIR}/runnable/hello-profile.d.trace.log
if [ ! -f ${tracelog} ]; then
    echo "missing file: ${tracelog}"
    exit 1
fi

rm ${RESULTS_DIR}/runnable/hello-profile.d.trace.{def,def2,log}

