make procchat
./procchat &
P_PID=$!

cd tests

gcc test_connection.c -o test_connection
./test_connection > log_a.out

sleep 2
DIFF=$(diff log_a.out test_connection.out)
if [ "$DIFF" != "" ]
then
    echo "[FAILED!]         Testing on test_connection"
    echo $DIFF
else
    echo "[PASSED]          Testing on test_connection"
fi
rm test_connection
rm -rf ../domain1
rm log_a.out
sleep 1

gcc test_say_a.c -o test_say_a
gcc test_say_b.c -o test_say_b
./test_say_a > log_a.out &
./test_say_b > log_b.out &

sleep 5
DIFF=$(diff log_a.out test_say_a.out)
if [ "$DIFF" != "" ]
then
    echo "[FAILED!]         Testing on test_say/recv_a"
    echo $DIFF
else
    echo "[PASSED]          Testing on test_say/recv_a"
fi
DIFF=$(diff log_b.out test_say_b.out)
if [ "$DIFF" != "" ]
then
    echo "[FAILED!]         Testing on test_say/recv_b"
    echo $DIFF
else
    echo "[PASSED]          Testing on test_say/recv_b"
fi
rm test_say_a test_say_b log_a.out log_b.out
rm -rf ../domain2
sleep 1

gcc test_saycont_a.c -o test_saycont_a
gcc test_saycont_b.c -o test_saycont_b
./test_saycont_a > log_a.out &
./test_saycont_b > log_b.out &

sleep 5
DIFF=$(diff log_a.out test_saycont_a.out)
if [ "$DIFF" != "" ]
then
    echo "[FAILED!]         Testing on test_saycont/recvcont_a"
    echo $DIFF
else
    echo "[PASSED]          Testing on test_saycont/recvcont_a"
fi
DIFF=$(diff log_b.out test_saycont_b.out)
if [ "$DIFF" != "" ]
then
    echo "[FAILED!]         Testing on test_saycont/recvcont_b"
    echo $DIFF
else
    echo "[PASSED]          Testing on test_saycont/recvcont_b"
fi
rm test_saycont_a test_saycont_b log_a.out log_b.out
rm -rf ../domain3
sleep 1

gcc test_ping_pong_a.c -o test_ping_pong_a
./test_ping_pong_a > log_a.out &

sleep 20
DIFF=$(diff log_a.out test_ping_pong_a.out)
if [ "$DIFF" != "" ]
then
    echo "[FAILED!]         Testing on test_ping_pong_a"
    echo $DIFF
else
    echo "[PASSED]          Testing on test_ping_pong_a"
fi
rm test_ping_pong_a log_a.out
rm -rf ../domain1
sleep 1

gcc test_ping_pong_b.c -o test_ping_pong_b
./test_ping_pong_b > log_a.out &

sleep 25
DIFF=$(diff log_a.out test_ping_pong_b.out)
if [ "$DIFF" != "" ]
then
    echo "[FAILED!]         Testing on test_ping_pong_b"
    echo $DIFF
else
    echo "[PASSED]          Testing on test_ping_pong_b"
fi
rm test_ping_pong_b log_a.out
rm -rf ../domain1
sleep 1

sleep 2
cd ..
echo $P_PID
kill $P_PID
rm gevent
rm procchat