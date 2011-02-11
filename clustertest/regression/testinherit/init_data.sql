insert into master (data) values ('master 000');
insert into sub1 (data) values ('sub1 a');
insert into sub2 (data) values ('sub1 b');
insert into sub3 (data) values ('sub1 c');


insert into regions (region_code, iso_country, region, city) values (1001, 'US', 'NY', 'New York City');
insert into regions (region_code, iso_country, region, city) values (1002, 'US', 'NY', 'Albany');
insert into regions (region_code, iso_country, region, city) values (1003, 'US', 'MA', 'Boston');
insert into regions (region_code, iso_country, region, city) values (1004, 'US', 'PH', 'Philadelphia');
insert into regions (region_code, iso_country, region, city) values (1005, 'US', 'DC', 'Washington');
insert into regions (region_code, iso_country, region, city) values (1006, 'US', 'GA', 'Atlanta');
insert into regions (region_code, iso_country, region, city) values (1007, 'US', 'FL', 'Miami');
insert into regions (region_code, iso_country, region, city) values (1008, 'US', 'ME', 'Portland');
insert into regions (region_code, iso_country, region, city) values (2000, 'US', 'OR', 'Portland');
insert into regions (region_code, iso_country, region, city) values (2001, 'US', 'CA', 'Los Angeles');
insert into regions (region_code, iso_country, region, city) values (2002, 'US', 'CA', 'San Francisco');
insert into regions (region_code, iso_country, region, city) values (2003, 'US', 'NV', 'Las Vegas');
insert into regions (region_code, iso_country, region, city) values (2004, 'US', 'CA', 'San Diego');
insert into regions (region_code, iso_country, region, city) values (2005, 'US', 'WA', 'Seattle');
insert into regions (region_code, iso_country, region, city) values (3000, 'CA', 'ON', 'Ottawa');
insert into regions (region_code, iso_country, region, city) values (3001, 'CA', 'ON', 'Toronto');
insert into regions (region_code, iso_country, region, city) values (3002, 'CA', 'NS', 'Halifax');
insert into regions (region_code, iso_country, region, city) values (3003, 'CA', 'AB', 'Calgary');
insert into regions (region_code, iso_country, region, city) values (3004, 'CA', 'BC', 'Vancouver');
insert into regions (region_code, iso_country, region, city) values (4000, 'GB', NULL, 'London');
insert into regions (region_code, iso_country, region, city) values (4001, 'DE', NULL, 'Munich');
insert into regions (region_code, iso_country, region, city) values (4002, 'FR', NULL, 'Paris');
insert into regions (region_code, iso_country, region, city) values (4003, 'IT', NULL, 'Rome');
insert into regions (region_code, iso_country, region, city) values (4004, 'EG', NULL, 'Cairo');
insert into regions (region_code, iso_country, region, city) values (4005, 'JP', NULL, 'Tokyo');
insert into regions (region_code, iso_country, region, city) values (4006, 'CH', NULL, 'Bejing');
insert into regions (region_code, iso_country, region, city) values (4007, 'AU', NULL, 'Melbourne');

insert into products (name, price) values ('widget', 275.00);
insert into products (name, price) values ('thingamajig', 17.55);
insert into products (name, price) values ('whatzit', 24.99);
insert into products (name, price) values ('dunno', 8.95);
insert into products (name, price) values ('one of those', 182.44);
insert into products (name, price) values ('thingamabob', 18.05);

select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from regions, products order by random() limit 3;
select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from regions, products order by random() limit 3;
select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from regions, products order by random() limit 3;
select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from regions, products order by random() limit 3;
select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from regions, products order by random() limit 3;
select purchase_product( region_code, product_id, (random()*5+random()*8+random()*7)::integer) from regions, products order by random() limit 3;


