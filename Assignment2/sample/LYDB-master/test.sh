./create  R1  3  4  "0,0:0,1:0,2:1,0:1,1:2,0"
./stats R1
./gendata 250 3 101 | ./insert R1
