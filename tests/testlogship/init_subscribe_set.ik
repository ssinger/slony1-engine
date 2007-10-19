subscribe set (id = 1, provider = 1, receiver = 2, forward = no);
echo 'sleep a couple of seconds...';
sleep (seconds = 2);
echo 'done sleeping...';
subscribe set (id = 1, provider = 1, receiver = 3, forward = yes);
echo 'sleep a couple of seconds...';
sleep (seconds = 2);
echo 'done sleeping...';
