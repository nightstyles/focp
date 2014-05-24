
mdb

use MDB;

create table MyProfile
(
	ont int32 not null,
	two string size=20 default='11',
	three int32,
	four string size=30
)capacity=1024;

create unique index MyProfileIndex on MyProfile(ont) by rbtree;

quit;
