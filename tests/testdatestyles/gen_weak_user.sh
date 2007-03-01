weakuser=$1;

for i in 1; do
   echo "grant select on table public.table${i} to ${weakuser};"
   echo "grant select on table public.table${i}_id_seq to ${weakuser};"
done