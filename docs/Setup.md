# Setup
## PostgreSQL

first sudo as postgres default superuser 'postgres' on bash

`sudo -i -u postgres`

adding an new new user by typing

`createuser -s engine`

create a new database

`createdb engine`

now start postgres and add password for these user

`psql`

write this statement

`ALTER USER engine WITH PASSWORD 'engine';`

For the tests you need a different database called `enginetest`.
