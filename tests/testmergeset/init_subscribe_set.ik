subscribe set (id = 1, provider = 1, receiver = 2, forward = yes);
wait for event(origin = all, confirmed = 2, wait on=1);
subscribe set (id = 1, provider = 1, receiver = 3, forward = no);
wait for event(origin = all, confirmed = 3, wait on=1);
subscribe set (id = 1, provider = 2, receiver = 4, forward = no);
wait for event(origin = all, confirmed = 4, wait on=1);
