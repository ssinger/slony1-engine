weakuser=$1;

for tbl in master sub1 sub2 sub3 regions products sales_data us_east us_west canada; do
   echo "grant select on table public.${tbl} to ${weakuser};"
done


for tbl in master sales_data; do
  echo "grant select on table public.${tbl}_id_seq to ${weakuser};"
done