weakuser=$1;

for i in table1 table2 table3 table4 utf8table; do
   echo "grant select on table public.${i} to ${weakuser};"
   echo "grant select on table public.table${i}_id_seq to ${weakuser};"
done
