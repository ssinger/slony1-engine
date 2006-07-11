subscribe set ( id = 1, provider = 1, receiver = 2, forward = no);
wait for event (origin=ALL, confirmed=ALL, wait on 1, timeout=200);
subscribe set ( id = 1, provider = 2, receiver = 4, forward = no);
wait for event (origin=ALL, confirmed=ALL, wait on 1, timeout=200);
subscribe set ( id = 2, provider = 1, receiver = 3, forward = no);
wait for event (origin=ALL, confirmed=ALL, wait on 1, timeout=200);
subscribe set ( id = 2, provider = 3, receiver = 4, forward = no);

