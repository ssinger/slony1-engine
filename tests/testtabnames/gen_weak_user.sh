weakuser=$1;

for i in 1 2 3; do
   echo "grant select on table public.table${i} to ${weakuser};"
   echo "grant select on table public.table${i}_id_seq to ${weakuser};"
done

echo "grant usage on schema \"Schema.name\" to ${weakuser};"
echo "grant usage on schema \"Studly Spacey Schema\" to ${weakuser};"
echo "grant select on table \"Schema.name\".\"user\" to ${weakuser};"
echo "grant select on table \"Schema.name\".\"Capital Idea\" to ${weakuser};"
echo "grant select on table public.evil_index_table to ${weakuser};"

