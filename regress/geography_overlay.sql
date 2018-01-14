﻿﻿SELECT 'two_holes_intersection', ST_AsText(ST_Intersection(
'POLYGON((10 35, 30 35, 30 20, 55 20, 55 0, 25 0, 25 20, 10 20, 10 35),(13 32, 13 22, 27 22, 27 32, 13 32),(28 17,52 17,52 3,28 3,28 17))'::geography,
'POLYGON((22.412109375 37.16031654673677,61.875 10.919617760254697,49.130859375 -3.162455530237848,29.267578125 -2.8991526985043006,21.62109375 11.609193407938955,20.390625 13.581920900545844,37.265625 23.805449612314625,14.58984375 17.727758609852284,6.328125 28.690587654250713,22.412109375 37.16031654673677))'::geography));

SELECT 'two_holes_combine', ST_AsText(ST_Intersection('POLYGON((5 36,38 36,38 5,5 5,5 36),(16 28,16 17,28 17,28 28,16 28))'::geography,
'POLYGON((21 41,-4 21,22 -2,44 23,21 41),(22 31,31 23,22 14,11 22,22 31))'::geography));

SELECT 'multihole', ST_AsText(ST_Intersection(
'POLYGON((170 40, 170 10, -160 10, -160 40, 170 40),(175 35, 175 15,-165 15, -165 35,  175 35))'::geography,
'POLYGON((161 25, -175 1, -151 25, -175 49, 161 25),(166 25, -175 44, -156 25, -175 6, 166 25),(174.1552734375 39.16414104768743,171.38671875 36.914764288955936,171.73828125 34.59704151614417,174.7705078125 36.35052700542763,176.7919921875 38.99357205820946,174.1552734375 39.16414104768743))'::geography));

SELECT 'multipolygon', ST_AsText(ST_Intersection(
'POLYGON((170 40, 170 10, -160 10, -160 40, 170 40),(175 35, -165 35, -165 15, 175 15, 175 35))'::geography,
'POLYGON((161 25, -175 1, -151 25, -175 49, 161 25),(166 25, -175 44, -156 25, -175 6, 166 25))'::geography));

SELECT 'intersection_at_existing_point', ST_AsText(ST_Union(
ST_Segmentize('POLYGON((160 10, 50 10, 50 20, 160 20, 160 10))'::geometry, 20)::geography,
'POLYGON((100 50,110 50,110 -50,100 -50,100 50))'::geography));

