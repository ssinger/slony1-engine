subscribe set (id=1, provider=1, receiver = 2, forward=yes);
wait for event (origin=2, confirmed=1);
sync(id=1);
wait for event (origin=1, confirmed=2);

subscribe set (id=2, provider=1, receiver = 2, forward=yes);
wait for event (origin=2, confirmed=1);
sync(id=1);
wait for event (origin=1, confirmed=2);

subscribe set (id=1, provider=2, receiver = 3, forward=yes);
wait for event (origin=3, confirmed=2);
sync(id=1);
wait for event (origin=1, confirmed=3);

subscribe set (id=2, provider=2, receiver = 3, forward=yes);
wait for event (origin=3, confirmed=2);
sync(id=1);
wait for event (origin=1, confirmed=3);
