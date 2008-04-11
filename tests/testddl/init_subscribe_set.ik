echo 'sleep a couple seconds';
sleep (seconds = 2);
subscribe set ( id = 1, provider = 1, receiver = 2, forward = no);
sync(id=1);
wait for event (origin=1, confirmed=2, wait on=1);
echo 'sleep a couple seconds';
sleep (seconds = 2);
subscribe set ( id = 1, provider = 1, receiver = 3, forward = no);
