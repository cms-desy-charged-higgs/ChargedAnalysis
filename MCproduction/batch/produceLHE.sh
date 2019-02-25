#!/bin/bash

./MG5_aMC_v2_6_4/bin/mg5_aMC command_$1_$2.txt 
gunzip Hc+hTol4b_MHc$1_Mh$2/Events/run_01/unweighted_events.lhe.gz 
mv Hc+hTol4b_MHc$1_Mh$2/Events/run_01/*.lhe ./ 
