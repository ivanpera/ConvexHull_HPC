#!/bin/bash

N_PROCS=4
REPORT_OMP="timereport-omp.out"
REPORT_MPI="timereport-mpi.out"
rm -f timereport-*

if (( $# < 2 ))
then
  echo "Usage: <input, mode> (where mode is 'mpi' or 'omp'"
  exit 1
fi

INPUT=$1

if [ "$2" == "mpi" ] 
then
  REPORT=${REPORT_MPI}
elif [ "$2" == "omp" ] 
then
  REPORT=${REPORT_OMP}
else
  echo "bad mode"
  exit 1
fi

for run in {1..5}
    do
        echo "################ RUN NUMBER ${run} ################" | tee -a ${REPORT}
        for ((procs=1; procs <= N_PROCS; procs++))
            do
                echo "      ****** THREAD NUMBER ${procs} ******" | tee -a ${REPORT}
		if [ "$2" == "mpi" ] 
                then
                  mpirun -n ${procs} mpi-convex-hull < ${INPUT} > /dev/null 2>> ${REPORT}
                else
		  OMP_NUM_THREADS=${procs} ./omp-convex-hull <  ${INPUT} > /dev/null 2>> ${REPORT}
                fi
        done
done
