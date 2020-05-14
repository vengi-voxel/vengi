# Setup {#setup}

## PostgreSQL

first sudo as postgres default superuser 'postgres' on bash

`sudo -i -u postgres`

adding an new new user by typing

`createuser -s vengi`

create a new database

`createdb vengi`

now start postgres and add password for these user

`psql`

write this statement

`ALTER USER vengi WITH PASSWORD 'engine';`

For the tests you need a different database called `enginetest`.

You can also use other user/password combinations by setting some cvars:

* **db_name**
* **db_host**
* **db_pw**
* **db_user**

See the [configuration](Configuration.md) documentation for more details.
