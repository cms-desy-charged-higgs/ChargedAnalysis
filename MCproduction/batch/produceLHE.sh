#!/bin/bash

./MG5_aMC_v2_6_4/bin/mg5_aMC command.txt 
gunzip Hc+hTol4b_MHc150_Mh75/Events/run_01/unweighted_events.lhe.gz 
mv Hc+hTol4b_MHc150_Mh75/Events/run_01/*.lhe ./ 
