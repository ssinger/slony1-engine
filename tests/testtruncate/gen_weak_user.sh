weakuser=$1;

for i in products customers orders line_items; do
   echo "grant select on table public.${i} to ${weakuser};"
   echo "grant select on table public.${i}_id_seq to ${weakuser};"
done
