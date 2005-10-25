failover (id = 2, backup node = 3);
wait for event (origin = 1, confirmed = 3);
#drop node (id = 2);
