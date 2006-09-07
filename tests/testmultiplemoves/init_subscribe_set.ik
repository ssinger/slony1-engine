subscribe set (id=1, provider=1, receiver = 2, forward=yes);
wait for event (origin=all, confirmed=all, wait on=1);
subscribe set (id=2, provider=1, receiver = 2, forward=yes);
wait for event (origin=all, confirmed=all, wait on=1);

subscribe set (id=1, provider=2, receiver = 3, forward=yes);
wait for event (origin=all, confirmed=all, wait on=2);
subscribe set (id=2, provider=2, receiver = 3, forward=yes);
wait for event (origin=all, confirmed=all, wait on=2);
