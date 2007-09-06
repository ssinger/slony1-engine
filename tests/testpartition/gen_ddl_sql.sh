year=$1
month=$2
cluster=$3

# We're looking for what month and year follow year/month by 1 month
nextmonth=$((${month} +1))
nextmonthsyear=${year}
if [ ${nextmonth} -ge 13 ]; then
   nextmonth=1
   nextmonthsyear=$((${year} +1))
fi

tableid=`printf "%04d%02d" ${year} ${month}`
   
echo "
create table sales_txns_${year}_${month} (
   check (trans_on >= '${year}-${month}-01' and trans_on < '${nextmonthsyear}-${nextmonth}-01'),
   primary key(id)
)  inherits (sales_txns);

create rule sales_${year}_${month} as on insert to sales_txns where trans_on >= '${year}-${month}-01' and trans_on < '${nextmonthsyear}-${nextmonth}-01'
do instead (
      insert into sales_txns_${year}_${month} select new.id, new.trans_on, new.region_code, new.product_id, new.quantity, new.amount;
);

select \"_${cluster}\".replicate_partition(${tableid}, 'public'::text, 'sales_txns_${year}_${month}'::text, NULL::text, 'Sales Partition for ${year} ${month}'::text);

"
