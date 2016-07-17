alter table pgbench_accounts 
	add column last_update timestamp without time zone;

with M as (
	select A.aid, coalesce(max(H.mtime), 'epoch') as max_mtime
		from pgbench_accounts A
		left join pgbench_history H on A.aid = H.aid
		group by A.aid
) update pgbench_accounts 
	set last_update = M.max_mtime
	from M
	where pgbench_accounts.aid = M.aid;

create function set_last_update () returns trigger as $$
begin
	new.last_update = current_timestamp;
	return new;
end;
$$ language plpgsql;

create trigger set_last_update 
	before insert or update on pgbench_accounts
	for each row execute procedure set_last_update();
