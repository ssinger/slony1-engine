-- Have two statements separated by a C-style string
select * from a;
/*
select count(distinct person_id)
from
    person
    join dictionary on dic_category_id=11
    left join person_settings on ps_person_id=person_id and ps_did=dic_id
where
    ps_person_id is null
-- only 9000!
*/
select * from b;
/************ *foooo*   /- *
/   here is a tremendously ugly C-style comment
* * * * * /
-- **{***(***[****/
select * from c;

/* lets have a comment and a  /* nested comment */ */
-- Force a query to be at the end...

create table foo;