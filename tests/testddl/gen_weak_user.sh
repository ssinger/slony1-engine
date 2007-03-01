weakuser=$1;

for i in 1 2 3 4; do
   echo "grant select on table public.table${i} to ${weakuser};"
   echo "grant select on table public.table${i}_id_seq to ${weakuser};"
done

echo "grant select on table public.billing_discount to ${weakuser};"
echo "grant select on table public.billing_discount_seq to ${weakuser};"
