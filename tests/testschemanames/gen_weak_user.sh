weakuser=$1;
echo "grant usage on schema \"foo\" to ${weakuser};"

for i in 1 2 3; do
   echo "grant select on table foo.table${i} to ${weakuser};"
   echo "grant select on table foo.table${i}_id_seq to ${weakuser};"
done

echo "grant usage on schema \"Schema.name\" to ${weakuser};"
echo "grant usage on schema \"Studly Spacey Schema\" to ${weakuser};"
echo "grant select on table \"Schema.name\".\"user\" to ${weakuser};"
echo "grant select on table \"Schema.name\".\"Capital Idea\" to ${weakuser};"
echo "grant select on table \"Studly Spacey Schema\".\"user\" to ${weakuser};"
echo "grant select on table \"Studly Spacey Schema\".\"a.periodic.sequence\" to ${weakuser};"
