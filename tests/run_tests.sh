#!/bin/bash

cd double_free;
./run_test.sh;

echo "";
echo "";

cd ../use_after_free;
./run_test.sh;

echo "";
echo "";

cd ../mem_leak;
./run_test.sh;

echo "";
echo "";

cd ../double_lock;
./run_test.sh;

echo "";
echo "";

cd ../use_after_return;
./run_test.sh;
