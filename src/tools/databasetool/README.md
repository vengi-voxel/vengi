# Table descriptions for the databasetool.

The databasetool binary will generate model files for the
table definitions in this file.

## Valid types
* password
* string
* int
* long
* timestamp

If no classname is specified, the table name will be used with `Model` as postfix.

**Example:**

Table user with be UserModel if no other classname was specified

All models will be put into a persistence namespace.

```
table <TABLENAME> {
  classname <STRING> (overrides the automatically determined name)
  namespace <STRING> (c++ namespace where the class is put into)
  field <FIELDNAME> {
    type <FIELDTYPE>
    notnull (optional)
    length <LENGTH> (optional)
    default <DEFAULTVALUE> (optional)
  }
  constraints {
    <FIELDNAME> unique
    <FIELDNAME> primarykey
    <FIELDNAME2> primarykey
    <FIELDNAME> autoincrement
    (<FIELD1>, <FIELD2>) unique
  }
}
```
